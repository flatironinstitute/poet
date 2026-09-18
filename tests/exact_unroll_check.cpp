/// \file exact_unroll_check.cpp
/// \brief Checks that `dynamic_for<U>` emits its body exactly `U` times, plus a set
/// of adjacent codegen contracts for `static_for`, `dispatch` and `dispatch_set`.
///
/// Compiles `exact_unroll_fixture.cpp` to assembly with the build's compiler under
/// three flag sets and counts the FMAs inside each fixture function, instruction or libm call.
/// A one-block range must also carry no branch. The constant-trip-count loop `naked`
/// is the positive control: under `-funroll-loops` it must read more than one FMA.
///
/// The same assembly also proves three more contracts: `static_for<0,8>` fully
/// unrolls (reuses the FMA/branch counter above); every `dispatch` one-instruction-body
/// function reaches its specialization with no `div` (one `imul` allowed only for the
/// strided case's compile-time reciprocal) and through at most one indirect call, while
/// every `dispatch_set` one reaches it through no indirect call at all; and a
/// `throw_on_no_match` miss throws out of the hit path rather than inline.
/// `naive_mod_table` is the positive control for the div/imul counters: `% 5` is not a
/// power of two, so it must show a div or an imul.
///
/// Usage: exact_unroll_check <compiler> <fixture.cpp> <include-dir> <work-dir> [compiler-arg...]

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace {

struct Case {
    const char *name;
    int bodies;
    bool straight;// no branch allowed
};

constexpr Case cases[] = { { "u1_blocks", 1, false },
    { "u2_blocks", 2, false },
    { "u4_blocks", 4, false },
    { "u8_blocks", 8, false },
    { "u2_tail", 2, false },
    { "u4_tail", 4, false },
    { "u2_one", 2, true },
    { "u4_one", 4, true },
    { "u8_one", 8, true },
    // static_for<0,8> is a compile-time unroll, not dynamic_for: reuses this fma/branch
    // counter to prove it fully unrolls too (8 fma, no loop).
    { "static_for8", 8, true } };

/// `dispatch`/`dispatch_set` functions with a one-instruction body: every specialization
/// is a handful of `mov`/`add`, so a `div`, an unexpected `imul`, or an extra indirect call
/// would mean the matcher degraded to runtime arithmetic or a double-dispatch instead of one
/// table probe. `imul_max` is 1 only for the strided case (the compile-time exact-stride
/// reciprocal); `indirect_max` is 0 for `dispatch_set`, which matches by compare chain, never
/// a table.
struct InstrCase {
    const char *name;
    int imul_max;
    int indirect_max;
};

constexpr InstrCase instr_cases[] = {
    { "dispatch_contig16", 0, 1 },// dispatch_param<inclusive_range<1,16>>: contiguous table
    { "dispatch_sparse5", 0, 1 },// dispatch_param over {1,2,4,8,16}: seq_lookup + table
    { "dispatch_2d", 0, 1 },// 2D {1,2,4}x{1,2}: fused flat-index table
    { "dispatch_strided8", 1, 1 },// fixed-gap stride: one exact-stride reciprocal imul
    { "dispatch_set4", 0, 0 },// dispatch_set, 4 tuples: at dispatch_set_linear_max, linear fold
    { "dispatch_set12", 0, 0 },// dispatch_set, 12 tuples: above it, sorted compare tree
};

struct Count {
    bool found = false;
    int bodies = 0;
    int branches = 0;
    int div = 0;// idiv/div: runtime division instead of a table probe
    int imul = 0;// imul: a compile-time reciprocal is the only case allowed one
    int indirect = 0;// call/jmp through a register: the dispatch table's own call
    bool call_before_ret = false;// a named (non-indirect) call before the function's first ret
};

/// FMAs, branches, divides, multiplies and calls between `name:` and its `.cfi_endproc`.
/// Instructions are indented; labels and directives are not counted.
auto count(const std::vector<std::string> &lines, const std::string &name) -> Count {
    static const std::regex fma(R"(fn?m(add|sub)|fml[as]|[\s,]_?fma\b)");
    static const std::regex branch(R"(^\s+(j[a-z]*|b|b\.[a-z]+|cbn?z|tbn?z)\s)");
    static const std::regex div(R"(^\s*i?div[a-z]*\s)");
    static const std::regex imul(R"(^\s*imul[a-z]*\s)");
    static const std::regex indirect(R"((call|jmp)[a-z]*\s+\*)");
    static const std::regex named_call(R"(^\s*call[a-z]*\s+(?!\*)\S)");
    static const std::regex ret(R"(^\s*ret[a-z]*\b)");
    Count out;
    auto line = lines.begin();
    for (; line != lines.end(); ++line) {
        if (line->rfind(name + ":", 0) == 0 || line->rfind("_" + name + ":", 0) == 0) { break; }
    }
    if (line == lines.end()) { return out; }
    out.found = true;
    bool seen_ret = false;
    for (++line; line != lines.end() && line->find(".cfi_endproc") == std::string::npos; ++line) {
        const std::size_t first = line->find_first_not_of(" \t");
        if (first == 0 || first == std::string::npos || (*line)[first] == '.') { continue; }
        if (std::regex_search(*line, fma)) {
            ++out.bodies;
        } else if (std::regex_search(*line, branch)) {
            ++out.branches;
        }
        if (std::regex_search(*line, div)) { ++out.div; }
        if (std::regex_search(*line, imul)) { ++out.imul; }
        if (std::regex_search(*line, indirect)) { ++out.indirect; }
        if (!seen_ret) {
            if (std::regex_search(*line, ret)) {
                seen_ret = true;
            } else if (std::regex_search(*line, named_call)) {
                out.call_before_ret = true;
            }
        }
    }
    return out;
}

/// True if some line anywhere in the file calls a symbol whose name contains "throw"
/// (case-insensitive): the Itanium ABI always raises via `__cxa_throw`, whether the
/// compiler keeps it inline after the hit path's `ret` (clang) or moves it to a
/// `.cold`/`.text.unlikely` block entirely (gcc) — either way the call shows up somewhere.
auto has_throw_call(const std::vector<std::string> &lines) -> bool {
    static const std::regex throw_call(R"(^\s*call[a-z]*\s+(?!\*)\S*throw)", std::regex::icase);
    for (const auto &line : lines) {
        if (std::regex_search(line, throw_call)) { return true; }
    }
    return false;
}

auto quoted(const std::string &text) -> std::string { return "\"" + text + "\""; }

}// namespace

auto main(int argc, char **argv) -> int {
    if (argc < 5) {
        std::cerr << "usage: exact_unroll_check <compiler> <fixture.cpp> <include-dir> <work-dir> [compiler-arg...]\n";
        return 2;
    }
    const std::string work_dir = argv[4];
    const std::string asm_path = work_dir + "/fixture.s";
    const std::string log = work_dir + "/compile.log";
    std::string command = quoted(argv[1]) + " -std=c++17 -S -o " + quoted(asm_path) + " -I" + quoted(argv[3]);
    for (int i = 5; i < argc; ++i) { command += " " + quoted(argv[i]); }
    command += " " + quoted(argv[2]) + " > " + quoted(log) + " 2>&1 ";
    std::filesystem::create_directories(work_dir);

    int failures = 0;
    for (const char *flags : { "-O2", "-O3", "-O3 -funroll-loops" }) {
        std::cout << "exact-unroll [" << flags << "] " << command << flags << "\n";
        if (std::system((command + flags).c_str()) != 0) {
            std::cout << "exact-unroll FAIL [" << flags << "] fixture did not compile:\n" << std::ifstream(log).rdbuf();
            ++failures;
            continue;
        }
        std::vector<std::string> lines;
        std::ifstream in(asm_path);
        for (std::string line; std::getline(in, line);) { lines.push_back(line); }

        for (const Case &item : cases) {
            const Count got = count(lines, item.name);
            const bool ok = got.found && got.bodies == item.bodies && (!item.straight || got.branches == 0);
            std::cout << "exact-unroll " << (ok ? "PASS" : "FAIL") << " [" << flags << "] " << item.name
                      << " bodies=" << got.bodies << " branches=" << got.branches << " expect bodies=" << item.bodies
                      << (item.straight ? " branches=0" : "") << (got.found ? "" : " (missing)") << "\n";
            failures += ok ? 0 : 1;
        }
        // The control proves the counter sees an unrolled loop where one exists.
        const Count naked = count(lines, "naked");
        const bool unrolling = std::string(flags).find("unroll") != std::string::npos;
        const bool ok = naked.found && (!unrolling || naked.bodies > 1);
        std::cout << "exact-unroll " << (ok ? "PASS" : "FAIL") << " [" << flags << "] naked bodies=" << naked.bodies
                  << (unrolling ? " expect bodies>1 (control)" : " (control, informative)") << "\n";
        failures += ok ? 0 : 1;

        // dispatch: no div ever; imul only for the strided reciprocal. dispatch_set: no
        // indirect call, since it matches by compare chain instead of a table.
        for (const InstrCase &c : instr_cases) {
            const Count got = count(lines, c.name);
            const bool instr_ok = got.found && got.div == 0 && got.imul <= c.imul_max && got.indirect <= c.indirect_max;
            std::cout << "exact-unroll " << (instr_ok ? "PASS" : "FAIL") << " [" << flags << "] " << c.name
                      << " div=" << got.div << " imul=" << got.imul << " indirect=" << got.indirect
                      << " expect div=0 imul<=" << c.imul_max << " indirect<=" << c.indirect_max
                      << (got.found ? "" : " (missing)") << "\n";
            failures += instr_ok ? 0 : 1;
        }

        // throw_on_no_match: the hit path never touches the throw machinery before its
        // ret, and the throw machinery exists somewhere (inline tail or out of line).
        const Count throw_fn = count(lines, "dispatch_throw16");
        const bool throws = has_throw_call(lines);
        const bool throw_ok = throw_fn.found && !throw_fn.call_before_ret && throws;
        std::cout << "exact-unroll " << (throw_ok ? "PASS" : "FAIL") << " [" << flags
                  << "] dispatch_throw16 call_before_ret=" << throw_fn.call_before_ret << " has_throw_call=" << throws
                  << " expect call_before_ret=0 has_throw_call=1" << (throw_fn.found ? "" : " (missing)") << "\n";
        failures += throw_ok ? 0 : 1;

        // Positive control for div/imul: `% 5` is not a power of two, so this must divide
        // or multiply by a magic-number reciprocal, proving the counters above actually fire.
        const Count control = count(lines, "naive_mod_table");
        const bool control_ok = control.found && (control.div + control.imul) > 0;
        std::cout << "exact-unroll " << (control_ok ? "PASS" : "FAIL") << " [" << flags
                  << "] naive_mod_table div=" << control.div << " imul=" << control.imul
                  << " expect div+imul>0 (control)" << (control.found ? "" : " (missing)") << "\n";
        failures += control_ok ? 0 : 1;
    }
    std::cout << "exact-unroll: " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}
