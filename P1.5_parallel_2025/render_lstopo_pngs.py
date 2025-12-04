import html
from pathlib import Path
import re
import cairosvg

# ---------- Settings ----------
WIDTH = 12000              # PNG width in pixels (height auto)
FONT_SIZE = 10             # pt
HEADER_H = 22              # px space for box header text
PADDING = 8                # px inner padding
TITLE_H = 60               # px title bar at top of each image
STROKE = "#222222"         # thin border color
BG = "#ffffff"             # white background

COLORS = {
    "machine": "#f5f5f5",
    "package": "#cfe8ff",
    "numa":    "#d8f5d0",
    "cache3":  "#8fc1e3",
    "cache":   "#bfe3ff",
    "core":    "#e6e1ff",
    "pu":      "#f3ecd4",
    "pcie":    "#ffcc80",
    "net":     "#ffe680",
    "disk":    "#ffb3c6",
    "other":   "#dddddd",
    "border":  "#333333",
    "text":    "#111111",
}

# ---------- Parse lstopo text ----------
def parse_hierarchy(lines):
    root = {"name": "Machine", "children": []}
    stack = [(0, root)]
    for raw in lines:
        if not raw.strip():
            continue
        indent = len(raw) - len(raw.lstrip(" "))
        name = raw.strip()
        node = {"name": name, "children": []}
        while stack and indent <= stack[-1][0]:
            stack.pop()
        parent = stack[-1][1] if stack else root
        parent["children"].append(node)
        stack.append((indent, node))
    return root

# ---------- Label handling ----------
def simplify_label(name: str) -> str:
    # Replace key tokens
    rep = {
        "Block(Disk)": "Disk",
        "HostBridge":  "Bridge",
        "PCIBridge":   "PCIe",
        "NUMANode":    "NUMA",
        "Package":     "Pkg",
        "OpenFabrics": "OFED",
        "NVMExp":      "NVMe",
    }
    for k, v in rep.items():
        name = name.replace(k, v)

    # Remove (P#x) and compress L# notation
    name = re.sub(r"\(P#\d+\)", "", name)        # remove (P#…)
    name = name.replace("L#","L")                # L#14 -> L14
    name = re.sub(r"\s+", " ", name).strip()
    return name

def classify(label: str) -> str:
    if label == "Machine": return "machine"
    if label.startswith("Pkg"): return "package"
    if label.startswith("NUMA"): return "numa"
    if label.startswith("L3"): return "cache3"
    if label.startswith("L2") or label.startswith("L1"): return "cache"
    if label.startswith("Core"): return "core"
    if label.startswith("PU"): return "pu"
    if "PCI" in label or "Bridge" in label: return "pcie"
    if "Net" in label or "InfiniBand" in label: return "net"
    if "Disk" in label or "NVMe" in label: return "disk"
    return "other"

# ---------- Layout ----------
def layout_children(n, avail_w, avail_h, node_type):
    # Choose child box min width by node type
    min_w = 180 if node_type in ("package","numa","cache3") else 140
    if node_type in ("cache","core","pu"):
        min_w = 120
    cols = max(1, int(avail_w // min_w))
    cols = min(cols, n) if n > 0 else 1
    rows = (n + cols - 1) // cols if n > 0 else 1
    child_w = avail_w / max(cols,1)
    child_h = avail_h / max(rows,1)
    return rows, cols, child_w, child_h

# ---------- SVG primitives ----------
def svg_rect(x, y, w, h, fill, stroke=STROKE, sw=1):
    return f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" height="{h:.2f}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>'

def svg_text(x, y, s, size=FONT_SIZE, weight="normal"):
    s = html.escape(s)
    w = ' font-weight="bold"' if weight=="bold" else ""
    return f'<text x="{x:.2f}" y="{y:.2f}" font-family="Helvetica,Arial" font-size="{size}" fill="{COLORS["text"]}"{w}>{s}</text>'

def draw_node(node, x, y, w, h, elems):
    label = simplify_label(node["name"])
    kind = classify(label)
    fill = COLORS.get(kind, COLORS["other"])

    # Box and header
    elems.append(svg_rect(x, y, w, h, fill))
    elems.append(svg_text(x + 4, y + HEADER_H - 6, label[:120]))

    children = node.get("children", [])
    if not children:
        return

    inner_x = x + PADDING
    inner_y = y + HEADER_H
    inner_w = max(1.0, w - 2 * PADDING)
    inner_h = max(1.0, h - HEADER_H - PADDING)

    rows, cols, cw, ch = layout_children(len(children), inner_w, inner_h, kind)
    for idx, child in enumerate(children):
        r = idx // cols
        c = idx % cols
        cx = inner_x + c * cw + 2
        cy = inner_y + r * ch + 2
        draw_node(child, cx, cy, max(4, cw - 4), max(4, ch - 4), elems)

def render_svg(root, title, width_px=WIDTH, aspect_ratio=1.6):
    # Height as a function of width (tall diagrams need room)
    height_px = int(width_px / aspect_ratio)

    elems = []
    # Background + outer border + title band
    elems.append(svg_rect(0, 0, width_px, height_px, BG, stroke=BG, sw=0))
    elems.append(svg_rect(12, 12, width_px-24, height_px-24, "none", stroke=STROKE, sw=1))
    elems.append(svg_text(32, TITLE_H - 18, title, size=24, weight="bold"))

    # Root drawing area beneath title
    x, y = 24, TITLE_H
    w, h = width_px - 48, height_px - TITLE_H - 24
    draw_node(root, x, y, w, h, elems)

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width_px}" height="{height_px}" viewBox="0 0 {width_px} {height_px}">',
        *elems,
        '</svg>'
    ]
    return "\n".join(svg)

# ---------- Build per-NUMA subtrees ----------
def find_numa_nodes(root):
    out = []
    def walk(node):
        label = simplify_label(node["name"])
        if label.startswith("NUMA"):
            out.append(node)
        for ch in node.get("children", []):
            walk(ch)
    walk(root)
    return out

def wrap_as_root(subtree, title):
    # Create a synthetic root to draw a single NUMA as the main content
    return {"name": "Machine", "children": [subtree]}

# ---------- Main ----------
def main():
    txt = Path("nodeArch.txt")
    if not txt.exists():
        raise SystemExit("nodeArch.txt not found. Place it next to this script.")
    lines = txt.read_text(encoding="utf-8", errors="ignore").splitlines()

    full_root = parse_hierarchy(lines)

    # Full system SVG → PNG
    svg_full = render_svg(full_root, "Full System Topology", width_px=WIDTH, aspect_ratio=1.6)
    Path("lstopo_full_box.svg").write_text(svg_full, encoding="utf-8")
    cairosvg.svg2png(bytestring=svg_full.encode("utf-8"),
                     write_to="lstopo_full_box.png",
                     output_width=WIDTH,
                     background_color=BG)

    # Per-NUMA SVGs → PNGs
    numas = find_numa_nodes(full_root)
    # Name NUMAs in order of appearance
    for idx, numa in enumerate(numas):
        title = f"NUMA Node {idx}"
        subtree_root = wrap_as_root(numa, title)
        svg = render_svg(subtree_root, title, width_px=WIDTH, aspect_ratio=1.2)
        out_svg = f"lstopo_NUMA{idx}.svg"
        out_png = f"lstopo_NUMA{idx}.png"
        Path(out_svg).write_text(svg, encoding="utf-8")
        cairosvg.svg2png(bytestring=svg.encode("utf-8"),
                         write_to=out_png,
                         output_width=WIDTH,
                         background_color=BG)

    print("Done.")
    print("Created:")
    print(" - lstopo_full_box.svg / lstopo_full_box.png")
    for i in range(len(numas)):
        print(f" - lstopo_NUMA{i}.svg / lstopo_NUMA{i}.png")

if __name__ == "__main__":
    main()

