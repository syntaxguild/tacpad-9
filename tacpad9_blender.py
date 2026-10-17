"""TacPad-9 / PRIMADON -- paste into Blender's Scripting tab and Run Script.

All dimensions below are millimetres; +Y is the header, +Z is up.
Change P and rerun to rebuild only this script's generated objects.
The enclosure is 85 x 120 x 22; the controls extend above that height.
This is a design model, without PCB mounts or production switch/stem fits.
"""

import math
import os
import struct
import tempfile

import bpy


P = {
    "width": 85.0, "length": 120.0, "height": 22.0,
    "wall": 2.4, "corner": 2.5, "bottom_thickness": 1.8,
    "recess_depth": 1.2, "recess_margin": 4.5,
    "pitch": 19.0, "key_size": 17.6, "key_height": 7.0,
    "key_radius": 1.4, "dish_depth": 0.75, "dish_radius": 32.0,
    "switch_hole": 14.2, "matrix_y": -13.0,
    "oled_width": 38.0, "oled_height": 16.0,
    "oled_x": -15.0, "header_y": 40.0, "glass_thickness": 1.0,
    "knob_x": 24.0, "knob_diameter": 16.0, "knob_height": 12.0,
    "knurl_teeth": 48, "knurl_depth": 0.30, "knurl_pitch": 1.8,
    "case_color": (0.64, 0.67, 0.69, 1.0),
    "key_color": (0.87, 0.88, 0.85, 1.0),
}

# Enable either flag for automatic export. Empty path = beside the .blend,
# or in the system temporary directory when the .blend has not been saved.
EXPORT_STL = False
EXPORT_OBJ = False
EXPORT_DIRECTORY = ""
TAG = "tacpad9_generated"


def activate(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def remove_object(obj):
    mesh = obj.data if obj.type == "MESH" else None
    bpy.data.objects.remove(obj, do_unlink=True)
    if mesh is not None and mesh.users == 0:
        bpy.data.meshes.remove(mesh)


def collection(name, parent):
    for child in parent.children:
        if child.get(TAG) and child.name == name:
            return child
    child = bpy.data.collections.new(name)
    child[TAG] = True
    parent.children.link(child)
    return child


def move_to(obj, target):
    for old in list(obj.users_collection):
        old.objects.unlink(obj)
    target.objects.link(obj)
    obj[TAG] = True
    return obj


def material(name, color, metallic=0.0, roughness=0.4, brushed=False):
    mat = next((m for m in bpy.data.materials
                if m.get(TAG) and m.name == name), None)
    if mat is None:
        mat = bpy.data.materials.new(name)
    mat[TAG] = True
    mat.diffuse_color = color
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    output = nodes.new("ShaderNodeOutputMaterial")
    mat.node_tree.links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    shader.inputs["Base Color"].default_value = color
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["IOR"].default_value = 1.5
    if brushed:
        for socket_name in ("Anisotropic IOR Level", "Anisotropic"):
            socket = shader.inputs.get(socket_name)
            if socket is not None:
                socket.default_value = 0.55
                break
        tex = nodes.new("ShaderNodeTexCoord")
        stretch = nodes.new("ShaderNodeVectorMath")
        stretch.operation = "MULTIPLY"
        stretch.inputs[1].default_value = (1.0, 1.0, 0.035)
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 180.0
        noise.inputs["Detail"].default_value = 2.0
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = 0.13
        bump.inputs["Distance"].default_value = 0.015
        links = mat.node_tree.links
        links.new(tex.outputs["Generated"], stretch.inputs[0])
        links.new(stretch.outputs["Vector"], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    return mat


def bevel(obj, width, segments=8, angle_limit=math.radians(30)):
    activate(obj)
    mod = obj.modifiers.new("Machined edge", "BEVEL")
    mod.width = width
    mod.segments = segments
    mod.limit_method = "ANGLE"
    mod.angle_limit = angle_limit
    bpy.ops.object.modifier_apply(modifier=mod.name)


def box(name, size, position, radius, target, mat=None):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=position)
    obj = move_to(bpy.context.object, target)
    obj.name = name
    obj.dimensions = size
    activate(obj)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if radius > 0:
        bevel(obj, radius)
    if mat is not None:
        obj.data.materials.append(mat)
    return obj


def subtract(obj, cutter):
    activate(obj)
    mod = obj.modifiers.new("Cut / " + cutter.name, "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.solver = "EXACT"
    mod.object = cutter
    bpy.ops.object.modifier_apply(modifier=mod.name)
    remove_object(cutter)


def finish(obj):
    for face in obj.data.polygons:
        face.use_smooth = True
    # Blender 4.1 removed this property; 4.0 needs it for custom normals.
    if hasattr(obj.data, "use_auto_smooth"):
        obj.data.use_auto_smooth = True
    activate(obj)
    mod = obj.modifiers.new("Surface normals", "WEIGHTED_NORMAL")
    mod.keep_sharp = True
    mod.weight = 50
    bpy.ops.object.modifier_apply(modifier=mod.name)


def knurled_knob(target, mat, z_bottom):
    """Closed mesh: two crossing helical groove families, not a bump map."""
    teeth = P["knurl_teeth"]
    around = teeth * 8
    height, radius = P["knob_height"], P["knob_diameter"] / 2
    edge = min(0.45, height * 0.1, radius * 0.15)
    rows = max(32, math.ceil(height / P["knurl_pitch"] * 12))
    profile = [(0.0, radius - edge, 0.0)]
    for row in range(rows + 1):
        z = edge + (height - 2 * edge) * row / rows
        fade = min(1.0, row / 2, (rows - row) / 2)
        profile.append((z, radius, fade))
    profile.append((height, radius - edge, 0.0))
    vertices, faces = [], []
    for z, r, fade in profile:
        for i in range(around):
            theta = 2 * math.pi * i / around
            phase = teeth * i / around
            a = abs((phase + z / P["knurl_pitch"]) % 1 - 0.5)
            b = abs((phase - z / P["knurl_pitch"]) % 1 - 0.5)
            groove = 1.0 - min(1.0, min(a, b) / 0.18)
            rr = r - fade * P["knurl_depth"] * groove
            vertices.append((rr * math.cos(theta), rr * math.sin(theta), z))
    for row in range(len(profile) - 1):
        for i in range(around):
            j = (i + 1) % around
            a, b = row * around, (row + 1) * around
            faces.append((a + i, a + j, b + j, b + i))
    faces.append(tuple(reversed(range(around))))
    last = (len(profile) - 1) * around
    faces.append(tuple(last + i for i in range(around)))
    mesh = bpy.data.meshes.new("TacPad / diamond knurl mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(f"Knob / D{2 * radius:g} x H{height:g} / diamond knurl", mesh)
    target.objects.link(obj)
    obj[TAG] = True
    obj.location = (P["knob_x"], P["header_y"], z_bottom)
    obj.data.materials.append(mat)
    # Flat facets preserve the actual diamond grip in both viewport and STL.
    return obj


def validate_parameters():
    for key, value in P.items():
        if isinstance(value, (int, float)):
            if not math.isfinite(value):
                raise ValueError("Non-finite parameter: " + key)
            if key not in {"matrix_y", "oled_x", "header_y", "knob_x"} and value <= 0:
                raise ValueError("Parameter must be positive: " + key)
    w, length, h, wall = (P[k] for k in ("width", "length", "height", "wall"))
    span = 2 * P["pitch"] + P["key_size"] + 2 * P["recess_margin"]
    if not 0 < P["recess_depth"] < wall < h - P["bottom_thickness"]:
        raise ValueError("Recess must leave a solid roof above the cavity.")
    if span / 2 + wall >= w / 2 or abs(P["matrix_y"]) + span / 2 + wall >= length / 2:
        raise ValueError("The recessed matrix does not fit inside the enclosure.")
    if not P["switch_hole"] < P["key_size"] < P["pitch"]:
        raise ValueError("Require switch_hole < key_size < pitch.")
    if P["switch_hole"] <= 0.4 or min(w, length) <= 2 * wall + 1:
        raise ValueError("Insufficient switch or enclosure clearance.")
    if not 0 < P["key_radius"] < min(P["key_size"], P["key_height"]) / 2:
        raise ValueError("Key radius is too large.")
    if not 0 < P["corner"] < min(w / 2, length / 2, h / 2):
        raise ValueError("Enclosure corner radius is too large.")
    if not 0 < P["dish_depth"] < min(P["key_height"] / 2, P["dish_radius"]):
        raise ValueError("Dish would cut through the keycap.")
    if 2 * P["dish_radius"] * P["dish_depth"] - P["dish_depth"] ** 2 >= (P["key_size"] / 2 - P["key_radius"]) ** 2:
        raise ValueError("Dish must stay inside the flat top of the keycap.")
    if not isinstance(P["knurl_teeth"], int) or P["knurl_teeth"] < 8:
        raise ValueError("knurl_teeth must be an integer >= 8.")
    if P["knurl_depth"] >= P["knob_diameter"] / 4:
        raise ValueError("Knurl depth is too large.")
    screen_left = P["oled_x"] - P["oled_width"] / 2
    screen_right = P["oled_x"] + P["oled_width"] / 2
    knob_r = P["knob_diameter"] / 2 + 0.4
    if (abs(P["oled_x"]) + P["oled_width"] / 2 + wall >= w / 2
            or abs(P["knob_x"]) + knob_r + wall >= w / 2
            or abs(P["header_y"]) + max(P["oled_height"] / 2, knob_r) + wall >= length / 2):
        raise ValueError("Header components do not fit inside the enclosure.")
    if not (screen_right + 1 < P["knob_x"] - knob_r or screen_left - 1 > P["knob_x"] + knob_r):
        raise ValueError("OLED and knob overlap.")
    if P["header_y"] - max(P["oled_height"] / 2, knob_r) <= P["matrix_y"] + span / 2 + 1:
        raise ValueError("Header overlaps the recessed key matrix.")
    if P["oled_width"] <= 8 or P["oled_height"] <= (P["oled_width"] - 6) / 4 + 1:
        raise ValueError("OLED glass must contain the 4:1 active display.")
    if P["glass_thickness"] >= wall:
        raise ValueError("Glass thickness must be smaller than the roof thickness.")


def export_model(directory="", stl=True, obj=False, separate_stl=False):
    """STL/OBJ numeric coordinates are mm. STL has no materials or unit metadata.

    export_model(separate_stl=True) writes one STL per component for slicing.
    Assembly STL retains separate bodies; it is not a Boolean union.
    Existing files with these generated names are overwritten.
    """
    base = bpy.path.abspath("//") if bpy.data.filepath else tempfile.gettempdir()
    folder = bpy.path.abspath(directory) if directory else os.path.join(base, "TacPad-9_exports")
    os.makedirs(folder, exist_ok=True)
    objects = sorted((o for o in bpy.context.scene.objects
                      if o.get(TAG) and o.type == "MESH"), key=lambda o: o.name)
    if not objects:
        raise RuntimeError("Run build() before exporting.")
    paths = []
    if stl:
        groups = [([o], o.name.split(" / ")[0]) for o in objects] if separate_stl else [(objects, "TacPad-9")]
        depsgraph = bpy.context.evaluated_depsgraph_get()
        # Binary STL via Python stdlib: also works in Blender 4.0, whose STL
        # exporter is an optional bundled add-on rather than a built-in operator.
        for group, name in groups:
            path = os.path.join(folder, name.replace("/", "_") + ".stl")
            count = 0
            with open(path, "wb") as stream:
                stream.write(b"TacPad-9 | millimetres".ljust(80, b"\0"))
                stream.write(struct.pack("<I", 0))
                for source in group:
                    evaluated = source.evaluated_get(depsgraph)
                    mesh = evaluated.to_mesh()
                    try:
                        mesh.calc_loop_triangles()
                        matrix = source.matrix_world
                        for triangle in mesh.loop_triangles:
                            a, b, c = (matrix @ mesh.vertices[i].co for i in triangle.vertices)
                            normal = (b - a).cross(c - a).normalized()
                            stream.write(struct.pack("<12fH", *normal, *a, *b, *c, 0))
                            count += 1
                    finally:
                        evaluated.to_mesh_clear()
                stream.seek(80)
                stream.write(struct.pack("<I", count))
            paths.append(path)
    if obj:
        selected = list(bpy.context.selected_objects)
        active = bpy.context.view_layer.objects.active
        try:
            bpy.ops.object.select_all(action="DESELECT")
            for source in objects:
                source.select_set(True)
            bpy.context.view_layer.objects.active = objects[0]
            path = os.path.join(folder, "TacPad-9.obj")
            bpy.ops.wm.obj_export(
                filepath=path, check_existing=False, export_selected_objects=True,
                apply_modifiers=True, global_scale=1.0,
                forward_axis="Y", up_axis="Z", export_materials=True,
            )
            paths.append(path)
        finally:
            bpy.ops.object.select_all(action="DESELECT")
            for source in selected:
                source.select_set(True)
            bpy.context.view_layer.objects.active = active
    for path in paths:
        print("TacPad-9 exported:", path)
    return paths


def build():
    if bpy.app.version < (4, 0, 0):
        raise RuntimeError("This script requires Blender 4.0 or newer.")
    validate_parameters()
    if bpy.context.object is not None and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    # Keep unrelated objects in the user's scene.
    for old in list(bpy.context.scene.objects):
        if old.get(TAG):
            remove_object(old)
    root = collection("TacPad-9", bpy.context.scene.collection)
    body_col = collection("01 / Enclosure", root)
    header_col = collection("02 / Header", root)
    keys_col = collection("03 / Keycaps", root)
    hardware_col = collection("04 / Switch placeholders", root)
    case_mat = material("TacPad / matte housing", P["case_color"], roughness=0.48)
    key_mat = material("TacPad / warm keycaps", P["key_color"], roughness=0.36)
    dark_mat = material("TacPad / graphite", (0.025, 0.032, 0.038, 1), roughness=0.55)
    glass_mat = material("TacPad / black glass", (0.006, 0.009, 0.012, 1), roughness=0.075)
    metal_mat = material("TacPad / brushed aluminium", (0.62, 0.65, 0.68, 1), metallic=1.0, roughness=0.25, brushed=True)
    active_mat = material("TacPad / OLED active 128x32", (0.012, 0.023, 0.027, 1), roughness=0.12)
    w, length, h, wall = (P[k] for k in ("width", "length", "height", "wall"))
    roof = h - wall
    floor = h - P["recess_depth"]
    span = 2 * P["pitch"] + P["key_size"] + 2 * P["recess_margin"]
    body = box(f"Enclosure / {w:g} x {length:g} x {h:g} mm", (w, length, h), (0, 0, h / 2), P["corner"], body_col, case_mat)
    cavity = box("Cutter / open underside", (w - 2 * wall, length - 2 * wall, roof + 2), (0, 0, (roof - 2) / 2), 0.8, body_col)
    subtract(body, cavity)
    recess = box("Cutter / recessed key deck", (span, span, P["recess_depth"] + 2), (0, P["matrix_y"], floor + (P["recess_depth"] + 2) / 2), 0.55, body_col)
    subtract(body, recess)
    opening = box("Cutter / OLED opening", (P["oled_width"], P["oled_height"], wall + 4), (P["oled_x"], P["header_y"], h - wall / 2), 0.65, body_col)
    subtract(body, opening)
    positions = [((column - 1) * P["pitch"], P["matrix_y"] + (1 - row) * P["pitch"])
                 for row in range(3) for column in range(3)]
    for index, (x, y) in enumerate(positions, 1):
        hole = box("Cutter / switch", (P["switch_hole"], P["switch_hole"], wall + 4), (x, y, h - wall / 2), 0.18, body_col)
        subtract(body, hole)
        size = P["switch_hole"] - 0.4
        box(f"Switch {index:02d} / placeholder", (size, size, 3.6), (x, y, floor - 0.2), 0.35, hardware_col, dark_mat)
    finish(body)
    plate = box("Bottom cover / inset", (w - 2 * wall - 0.5, length - 2 * wall - 0.5, P["bottom_thickness"]), (0, 0, P["bottom_thickness"] / 2), 0.5, body_col, case_mat)
    finish(plate)
    glass_top = h - 0.35
    glass = box("OLED glass / recessed", (P["oled_width"] - 0.4, P["oled_height"] - 0.4, P["glass_thickness"]), (P["oled_x"], P["header_y"], glass_top - P["glass_thickness"] / 2), 0.18, header_col, glass_mat)
    finish(glass)
    active_w = P["oled_width"] - 6
    box("OLED active area / 4 to 1", (active_w, active_w / 4, 0.04), (P["oled_x"], P["header_y"], glass_top + 0.015), 0.015, header_col, active_mat)
    bpy.ops.mesh.primitive_cylinder_add(vertices=96, radius=P["knob_diameter"] / 2 + 0.4, depth=0.5, location=(P["knob_x"], P["header_y"], h + 0.25))
    collar = move_to(bpy.context.object, header_col)
    collar.name = "Knob collar"
    collar.data.materials.append(dark_mat)
    bevel(collar, 0.12, 4)
    finish(collar)
    knob_bottom = h + 0.5
    knurled_knob(header_col, metal_mat, knob_bottom)
    box("Knob index", (0.65, P["knob_diameter"] * 0.18, 0.06), (P["knob_x"], P["header_y"] + P["knob_diameter"] * 0.29, knob_bottom + P["knob_height"] + 0.02), 0.025, header_col, dark_mat)
    key_bottom = floor + 1.4
    key_top = key_bottom + P["key_height"]
    cap = box("Keycap 01", (P["key_size"], P["key_size"], P["key_height"]), (0, 0, key_bottom + P["key_height"] / 2), P["key_radius"], keys_col, key_mat)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=192, ring_count=192, radius=P["dish_radius"], location=(0, 0, key_top + P["dish_radius"] - P["dish_depth"]))
    sphere = move_to(bpy.context.object, keys_col)
    sphere.name = "Cutter / ergonomic dish"
    subtract(cap, sphere)
    bevel(cap, 0.16, 4, math.radians(5))
    finish(cap)
    cap.location.x, cap.location.y = positions[0]
    for index, (x, y) in enumerate(positions[1:], 2):
        other = cap.copy()
        other.data = cap.data  # shared finished mesh; no nine repeated booleans
        other.name = f"Keycap {index:02d}"
        keys_col.objects.link(other)
        other.location.x, other.location.y = x, y
    assembly = bpy.data.objects.new("TacPad-9 / Assembly", None)
    root.objects.link(assembly)
    assembly[TAG] = True
    assembly.empty_display_size = 10
    for source in root.all_objects:
        if source.type == "MESH":
            source.parent = assembly
    units = bpy.context.scene.unit_settings
    units.system, units.scale_length, units.length_unit = "METRIC", 0.001, "MILLIMETERS"
    bpy.context.view_layer.update()
    # Small executable acceptance check, also valid when running headlessly.
    assert len(keys_col.objects) == 9, "Expected a 3 x 3 key matrix"
    assert all(abs(a - b) < 0.01 for a, b in zip(body.dimensions, (w, length, h))), "Enclosure dimensions changed"
    bpy.ops.object.select_all(action="DESELECT")
    for source in root.all_objects:
        if source.type == "MESH":
            source.select_set(True)
    bpy.context.view_layer.objects.active = body
    # Frame selection only in existing 3D viewports, including the Scripting tab.
    if not bpy.app.background and bpy.context.screen is not None:
        for area in bpy.context.screen.areas:
            if area.type == "VIEW_3D":
                area.spaces.active.clip_end = 10000
                for region in area.regions:
                    if region.type == "WINDOW":
                        with bpy.context.temp_override(area=area, region=region):
                            bpy.ops.view3d.view_selected(use_all_regions=False)
                        break
    if EXPORT_STL or EXPORT_OBJ:
        export_model(EXPORT_DIRECTORY, EXPORT_STL, EXPORT_OBJ)
    print(f"TacPad-9 built: enclosure {w:g} x {length:g} x {h:g} mm; nine keys on {P['pitch']:g} mm pitch.")
    return root


if __name__ == "__main__":
    build()
