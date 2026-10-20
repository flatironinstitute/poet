import glob
import os
import re
import xml.etree.ElementTree as ET

from docutils.nodes import make_id

# Project information
project = "POET"
copyright = "2025-2026, POET Authors"
author = "Marco Barbone"

# Extensions
extensions = [
    "breathe",
    "exhale",
    "myst_parser",
    "sphinx_rtd_theme",
]

# Breathe configuration
# Use environment variable if available (set by CMake), otherwise default to build/
doxygen_xml = os.environ.get("DOXYGEN_XML_OUTPUT", "../build/docs/xml")
breathe_projects = {"POET": os.path.abspath(doxygen_xml)}
breathe_default_project = "POET"

# Exhale configuration
exhale_args = {
    "containmentFolder": "./api",
    "rootFileName": "library_root.rst",
    "doxygenStripFromPath": os.path.abspath(".."),
    "rootFileTitle": "POET API Reference",
    "createTreeView": True,
    "exhaleExecutesDoxygen": False,
    # Keep generated pages focused on public API.
    "listingExclude": [
        r".*::detail::.*",
        r".*\\bdetail\\b.*",
        r".*dispatch_set::convert_tuple.*",
        r"^poet::dispatch$",
        r"^poet::dynamic_for$",
        r"^poet::static_for$",
    ],
    # Avoid brittle overload resolution pages from generated function entries.
    "unabridgedOrphanKinds": [
        "namespace",
        "class",
        "struct",
        "enum",
        "typedef",
        "variable",
    ],
}

# Theme
html_theme = "sphinx_rtd_theme"
html_theme_options = {
    "navigation_depth": 4,
    "collapse_navigation": False,
    "sticky_navigation": True,
}

# RTD theme reads these to render the "Edit on GitHub" link on every page.
html_context = {
    "display_github": True,
    "github_user": "flatironinstitute",
    "github_repo": "poet",
    "github_version": "main",
    "conf_py_path": "/docs/",
}

# Breathe/Exhale misparse the heavily-overloaded templates (dispatch, static_for); warnings are noise.
suppress_warnings = [
    "docutils",
    "cpp.duplicate_declaration",
    "duplicate_declaration.cpp",
    "toc.not_included",
]


def _strip_private_macro_refs(app, env, docnames):
    api_dir = os.path.join(os.path.dirname(__file__), "api")
    if not os.path.isdir(api_dir):
        return

    blocked = (
        "``poet/core/macros.hpp``",
        "``poet/core/undef_macros.hpp``",
    )

    for rst_path in glob.glob(os.path.join(api_dir, "file_include_*.rst")):
        with open(rst_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        filtered = [
            line for line in lines if not any(token in line for token in blocked)
        ]
        if filtered != lines:
            with open(rst_path, "w", encoding="utf-8") as f:
                f.writelines(filtered)


def _fix_exhale_output(app, env, docnames):
    # Exhale slugs tree-view fragments itself and keeps "." and a trailing "-",
    # so rewrite each one to the id docutils gives the target section.
    # A title that ends in "_" (tuple_) parses as a reference: escape the "_".
    api_dir = os.path.join(os.path.dirname(__file__), "api")
    if not os.path.isdir(api_dir):
        return

    for path in glob.glob(os.path.join(api_dir, "*_view_hierarchy.rst.include")):
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
        fixed = re.sub(
            r'(href="[^"#]+#)([^"]+)"',
            lambda m: m.group(1) + make_id(m.group(2)) + '"',
            text,
        )
        if fixed != text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(fixed)

    for path in glob.glob(os.path.join(api_dir, "*.rst")):
        with open(path, "r", encoding="utf-8") as f:
            lines = f.readlines()
        changed = False
        for i in range(len(lines) - 1):
            title, under = lines[i].rstrip("\n"), lines[i + 1].rstrip("\n")
            if (
                title.endswith("_")
                and not title.endswith("\\_")
                and under
                and set(under) == {"="}
                and len(under) >= len(title)
            ):
                lines[i] = title[:-1] + "\\_\n"
                lines[i + 1] = "=" * (len(title) + 1) + "\n"
                changed = True
        if changed:
            with open(path, "w", encoding="utf-8") as f:
                f.writelines(lines)


def _fix_doxygen_xml(app, env, docnames):
    # Doxygen writes `auto f(...) -> decltype(auto)` as `decltype(auto) f(...)`.
    # Breathe splits a signature at its first "(", so no such overload resolves:
    # put the trailing return type back.
    for path in glob.glob(os.path.join(breathe_projects["POET"], "*.xml")):
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
        fixed = re.sub(
            r"<type>decltype\(auto\)</type>(\s*<definition>[^<]*</definition>\s*<argsstring>[^<]*)</argsstring>",
            r"<type>auto</type>\1 -&gt; decltype(auto)</argsstring>",
            text,
        )
        if fixed != text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(fixed)


def _rst_text(para):
    parts = [para.text or ""]
    for child in para:
        text = "".join(child.itertext())
        parts.append(f"``{text}``" if child.tag == "computeroutput" else text)
        parts.append(child.tail or "")
    return " ".join("".join(parts).split())


def _cpp_function(member):
    # A cpp:function directive made from one Doxygen memberdef.
    def text(e):
        return " ".join("".join(e.itertext()).split())

    tparams = []
    for p in member.findall("templateparamlist/param"):
        tparam = " ".join(
            text(p.find(t)) for t in ("type", "declname") if p.find(t) is not None
        )
        if p.find("defval") is not None:
            tparam += " = " + text(p.find("defval"))
        tparams.append(tparam)
    signature = f"template<{', '.join(tparams)}> " if tparams else ""
    signature += f"{text(member.find('type'))} {member.findtext('qualifiedname')}{member.findtext('argsstring')}"

    body, fields = [], []
    for para in member.findall("briefdescription/para") + member.findall(
        "detaileddescription/para"
    ):
        for plist in para.findall("parameterlist"):
            role = {"templateparam": "tparam", "param": "param"}[plist.get("kind")]
            for item in plist.findall("parameteritem"):
                name = text(item.find("parameternamelist"))
                fields.append(
                    f":{role} {name}: {_rst_text(item.find('parameterdescription/para'))}"
                )
            para.remove(plist)
        if _rst_text(para):
            body.append(_rst_text(para))
    lines = [f".. cpp:function:: {signature}", ""]
    lines += [f"   {line}\n" for line in body]
    lines += [f"   {line}" for line in fields]
    return "\n".join(lines) + "\n"


def _render_ambiguous_overloads(app, env, docnames):
    # Breathe cannot tell apart overloads whose argument lists differ only in
    # template parameters (dynamic_for with and without Step), so a page for
    # such an overload renders its own memberdef instead.
    api_dir = os.path.join(os.path.dirname(__file__), "api")
    directive = re.compile(r"^\.\. doxygenfunction:: (.+)\n   :project: POET$", re.M)
    pages = {}
    for path in glob.glob(os.path.join(api_dir, "function_*.rst")):
        with open(path, "r", encoding="utf-8") as f:
            match = directive.search(f.read())
        if match:
            pages.setdefault(match.group(1), []).append(path)
    ambiguous = [path for paths in pages.values() if len(paths) > 1 for path in paths]
    if not ambiguous:
        return

    members = {}
    for path in glob.glob(os.path.join(breathe_projects["POET"], "*.xml")):
        for member in ET.parse(path).getroot().iter("memberdef"):
            members[member.get("id")] = member
    for path in ambiguous:
        refid = os.path.basename(path)[len("function_") : -len(".rst")]
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
        with open(path, "w", encoding="utf-8") as f:
            f.write(directive.sub(lambda m: _cpp_function(members[refid]), text))


def _hide_edit_link_without_source(app, pagename, templatename, context, doctree):
    # Exhale writes api/ at build time (gitignored) and genindex has no source,
    # so "Edit on GitHub" would point at a file that is not in the repository.
    if doctree is None or pagename.startswith("api/"):
        context["display_github"] = False


def setup(app):
    app.connect("env-before-read-docs", _strip_private_macro_refs)
    app.connect("env-before-read-docs", _fix_exhale_output)
    app.connect("env-before-read-docs", _fix_doxygen_xml)
    app.connect("env-before-read-docs", _render_ambiguous_overloads)
    app.connect("html-page-context", _hide_edit_link_without_source)
