"""Build and photograph Tacpad 9 / MODEL NINE by Primadon in Blender 4.x.

Run beside tacpad9_blender.py: blender -b --python tacpad9_render.py
Use -- --preview for a quick lighting proof, or -- --setup-only to skip renders.
Final .blend, 1:1 / 16:9 PNGs and mesh exports go to renders/primadon/.
"""

import math
import os
from pathlib import Path
import runpy
import sys
import tempfile

import bpy
from mathutils import Vector


HERE = Path(__file__).resolve().parent
OUT = HERE / "renders" / "primadon"
LOGO_PATH = HERE / "assets" / "primadon-logo.png"
PREVIEW = "--preview" in sys.argv
SETUP_ONLY = "--setup-only" in sys.argv
STUDIO_TAG = "tacpad9_studio"


def aim(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def studio_object(obj, target):
    for col in list(obj.users_collection):
        col.objects.unlink(obj)
    target.objects.link(obj)
    obj[STUDIO_TAG] = True
    return obj


def shader_of(mat):
    return next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")


def microtexture(mat, scale, strength, distance):
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    tex = nodes.new("ShaderNodeTexNoise")
    tex.inputs["Scale"].default_value = scale
    tex.inputs["Detail"].default_value = 2
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = strength
    bump.inputs["Distance"].default_value = distance
    links.new(tex.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], shader_of(mat).inputs["Normal"])


def load_font(filename):
    path = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts" / filename
    return bpy.data.fonts.load(str(path)) if path.exists() else bpy.data.fonts.get("Bfont")


def label(body, location, size, mat, font, target, align="CENTER", surface=None):
    curve = bpy.data.curves.new("Print / " + body, "FONT")
    curve.body, curve.size, curve.align_x = body, size, align
    curve.align_y = "CENTER"
    curve.font = font
    curve.resolution_u = 12
    curve.space_character = 1.12
    obj = bpy.data.objects.new("Print / " + body, curve)
    target.objects.link(obj)
    obj[STUDIO_TAG] = True
    obj.location = location
    curve.materials.append(mat)
    if surface is not None:
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.convert(target="MESH")
        mod = obj.modifiers.new("Print follows ergonomic dish", "SHRINKWRAP")
        mod.target = surface
        mod.wrap_method = "PROJECT"
        mod.use_project_z = True
        mod.use_negative_direction = True
        mod.use_positive_direction = False
        mod.offset = 0.016
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def team_logo(target, location, height=10.4):
    """Use the original RGBA file as pad print; keep crisp pixel-art edges."""
    image = bpy.data.images.load(str(LOGO_PATH), check_existing=True)
    image.colorspace_settings.name = "sRGB"
    width_px, height_px = image.size
    alpha = list(image.pixels)[3::4]
    visible = [i for i, a in enumerate(alpha) if a > 0.01]
    if not visible:
        raise ValueError("The Primadon logo is completely transparent.")
    xmin = min(i % width_px for i in visible)
    xmax = max(i % width_px for i in visible) + 1
    ymin = min(i // width_px for i in visible)
    ymax = max(i // width_px for i in visible) + 1
    width = height * (xmax - xmin) / (ymax - ymin)
    mesh = bpy.data.meshes.new("Primadon / pixel mascot print")
    mesh.from_pydata([(-width/2, -height/2, 0), (width/2, -height/2, 0),
                      (width/2, height/2, 0), (-width/2, height/2, 0)], [], [(0,1,2,3)])
    uv = mesh.uv_layers.new(name="Original logo / visible alpha bounds")
    coords = [(xmin/width_px, ymin/height_px), (xmax/width_px, ymin/height_px),
              (xmax/width_px, ymax/height_px), (xmin/width_px, ymax/height_px)]
    for loop, coord in zip(uv.data, coords):
        loop.uv = coord
    mesh.update()
    mat = bpy.data.materials.new("Primadon / original full-colour logo")
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    texture.extension = "CLIP"
    ink = nodes.new("ShaderNodeBsdfPrincipled")
    ink.inputs["Roughness"].default_value = 0.48
    ink.inputs["Specular IOR Level"].default_value = 0.25
    transparent = nodes.new("ShaderNodeBsdfTransparent")
    mix = nodes.new("ShaderNodeMixShader")
    output = nodes.new("ShaderNodeOutputMaterial")
    links.new(texture.outputs["Color"], ink.inputs["Base Color"])
    links.new(texture.outputs["Alpha"], mix.inputs[0])
    links.new(transparent.outputs[0], mix.inputs[1])
    links.new(ink.outputs["BSDF"], mix.inputs[2])
    links.new(mix.outputs[0], output.inputs["Surface"])
    obj = bpy.data.objects.new("Primadon / supplied team logo", mesh)
    target.objects.link(obj)
    obj[STUDIO_TAG] = True
    obj.location = location
    mesh.materials.append(mat)
    image.pack()
    return obj


def oled_pixels(target, mat, p):
    # Native 128 x 32 raster, each pixel is a tiny emissive quad, 0.25 mm pitch.
    glyphs = {
        "P": ("11110", "10001", "10001", "11110", "10000", "10000", "10000"),
        "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
        "I": ("111", "010", "010", "010", "010", "010", "111"),
        "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
        "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
        "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
        "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
        "N": ("10001", "11001", "10101", "10011", "10001", "10001", "10001"),
        "C": ("01111", "10000", "10000", "10000", "10000", "10000", "01111"),
        "V": ("10001", "10001", "10001", "10001", "10001", "01010", "00100"),
        "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
        "5": ("11111", "10000", "10000", "11110", "00001", "00001", "11110"),
        "9": ("01110", "10001", "10001", "01111", "00001", "00001", "11110"),
        "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
        "%": ("11001", "11010", "00100", "00100", "01000", "10110", "00110"),
        " ": ("000",) * 7,
    }
    pixels = set()

    def text(body, x, y):
        for ch in body:
            glyph = glyphs[ch]
            for row, line in enumerate(glyph):
                for column, bit in enumerate(line):
                    if bit == "1":
                        pixels.add((x + column, y + row))
            x += len(glyph[0]) + 1

    text("TACPAD 9", 3, 3)
    text("MACRO", 94, 3)
    text("VOL 55%", 3, 22)
    for x in range(3, 125):
        if x % 2 == 0:
            pixels.add((x, 15))
    for x in range(64, 124):
        for y in range(22, 28):
            if x in (64, 123) or y in (22, 27) or (67 <= x <= 96 and 24 <= y <= 25):
                pixels.add((x, y))
    pitch = (p["oled_width"] - 6) / 128
    left, top = p["oled_x"] - 64 * pitch, p["header_y"] + 16 * pitch
    z = p["height"] - 0.35 + 0.048
    vertices, faces = [], []
    for x, y in sorted(pixels):
        x0, y0 = left + (x + 0.10) * pitch, top - (y + 0.10) * pitch
        s = pitch * 0.80
        index = len(vertices)
        vertices.extend(((x0, y0-s, z), (x0+s, y0-s, z), (x0+s, y0, z), (x0, y0, z)))
        faces.append((index, index+1, index+2, index+3))
    mesh = bpy.data.meshes.new("OLED / 128x32 luminous pixels")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("OLED / Tacpad 9 + MACRO + volume 55%", mesh)
    target.objects.link(obj)
    obj[STUDIO_TAG] = True
    mesh.materials.append(mat)


def area_light(name, location, target, power, size, color, collection, ratio=1):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = power
    data.shape = "RECTANGLE"
    data.size, data.size_y = size, size * ratio
    data.color = color
    obj = bpy.data.objects.new(name, data)
    collection.objects.link(obj)
    obj[STUDIO_TAG] = True
    obj.location = location
    aim(obj, target)
    return obj


def camera(name, location, target, collection, lens=65, ortho=None):
    data = bpy.data.cameras.new(name)
    obj = bpy.data.objects.new(name, data)
    collection.objects.link(obj)
    obj[STUDIO_TAG] = True
    obj.location = location
    data.lens, data.clip_start, data.clip_end = lens, 0.1, 10000
    if ortho is not None:
        data.type, data.ortho_scale = "ORTHO", ortho
    aim(obj, target)
    return obj


def build_studio():
    model = runpy.run_path(str(HERE / "tacpad9_blender.py"), run_name="tacpad9_model")
    p = model["P"]
    p["case_color"] = (0.53, 0.56, 0.57, 1)
    p["key_color"] = (0.76, 0.75, 0.69, 1)
    scene = bpy.data.scenes.new("Tacpad 9 / Primadon Studio")
    scene[STUDIO_TAG] = True
    bpy.context.window.scene = scene
    root = model["build"]()
    stage = bpy.data.collections.new("Studio / lights, cameras and background")
    scene.collection.children.link(stage)
    print_col = bpy.data.collections.new("Presentation / pad print and OLED pixels")
    root.children.link(print_col)
    hardware = model["collection"]("05 / Finish hardware", root)
    mat = model["material"]
    case = bpy.data.materials.get("TacPad / matte housing")
    keys = bpy.data.materials.get("TacPad / warm keycaps")
    shader_of(case).inputs["Roughness"].default_value = 0.34
    shader_of(keys).inputs["Roughness"].default_value = 0.43
    shader_of(keys).inputs["Specular IOR Level"].default_value = 0.28
    microtexture(case, 190, 0.10, 0.018)
    microtexture(keys, 140, 0.075, 0.012)
    ink = mat("Studio / charcoal pad print", (0.018, 0.026, 0.029, 1), roughness=0.48)
    accent = mat("Studio / Primadon moss green keycap", (0.075, 0.17, 0.085, 1), roughness=0.43)
    shader_of(accent).inputs["Specular IOR Level"].default_value = 0.28
    microtexture(accent, 140, 0.075, 0.012)
    accent_ink = mat("Studio / ivory pad print", (0.88, 0.83, 0.70, 1), roughness=0.45)
    steel = mat("Studio / blackened fasteners", (0.035, 0.042, 0.046, 1), metallic=0.8, roughness=0.31)
    rubber = mat("Studio / silicone feet", (0.012, 0.015, 0.017, 1), roughness=0.74)
    emit = mat("Studio / cool white OLED", (0.45, 0.80, 0.72, 1), roughness=0.28)
    shader_of(emit).inputs["Emission Color"].default_value = (0.48, 0.88, 0.77, 1)
    shader_of(emit).inputs["Emission Strength"].default_value = 2.0
    display = bpy.data.materials.get("TacPad / OLED active 128x32")
    shader_of(display).inputs["Base Color"].default_value = (0.0015, 0.003, 0.004, 1)
    shader_of(display).inputs["Roughness"].default_value = 0.18
    mono, sans = load_font("consola.ttf"), load_font("arialbd.ttf")
    cap_objects = sorted((o for o in root.all_objects if o.name.startswith("Keycap")), key=lambda o:o.name)
    names = ["COPY", "PASTE", "BUILD", "BLENDER", "UNDO", "REDO", "PLAY", "MUTE", "TERMINAL"]
    top = p["height"] - p["recess_depth"] + 1.4 + p["key_height"]
    for i, (cap, name) in enumerate(zip(cap_objects, names), 1):
        if i == 5:
            cap.data = cap.data.copy()
            cap.data.materials[0] = accent
        color = accent_ink if i == 5 else ink
        label(name, (cap.location.x, cap.location.y-1.0, top+0.5), 1.95 if len(name) < 7 else 1.65, color, mono, print_col, surface=cap)
        label(f"{i:02d}", (cap.location.x-5.0, cap.location.y+4.5, top+0.5), 1.05, color, mono, print_col, surface=cap)
    team_logo(print_col, (-28.0, -52.1, p["height"]+0.030))
    label("Tacpad 9", (-19.3, -50.6, p["height"]+0.025), 3.7, ink, sans, print_col, align="LEFT")
    label("MODEL NINE", (-19.1, -54.1, p["height"]+0.025), 1.10, ink, mono, print_col, align="LEFT")
    label("PRIMADON", (28.7, -51.7, p["height"]+0.025), 1.50, ink, sans, print_col, align="RIGHT")
    label("DESIGNED BY THE TEAM", (28.6, -54.0, p["height"]+0.025), 0.70, ink, mono, print_col, align="RIGHT")
    oled_pixels(print_col, emit, p)
    assembly = next(o for o in root.all_objects if o.type == "EMPTY")
    for x in (-35.5, 35.5):
        for y in (-52.5, 52.5):
            bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=1.40, depth=0.28, location=(x, y, p["height"]-0.02))
            screw = model["move_to"](bpy.context.object, hardware)
            screw.name = f"Fastener / {x:g} {y:g}"
            screw.data.materials.append(steel)
            model["bevel"](screw, 0.10, 4)
            slot = model["box"]("Cutter / screw drive", (1.65, 0.35, 0.3), (x, y, p["height"]+0.15), 0.08, hardware)
            model["subtract"](screw, slot)
            model["finish"](screw)
            screw.parent = assembly
            bpy.ops.mesh.primitive_cylinder_add(vertices=64, radius=4, depth=1.1, location=(x, y, -0.45))
            foot = model["move_to"](bpy.context.object, hardware)
            foot.name = f"Silicone foot / {x:g} {y:g}"
            foot.data.materials.append(rubber)
            model["bevel"](foot, 0.35, 6)
            model["finish"](foot)
            foot.parent = assembly
    for obj in print_col.objects:
        obj.parent = assembly
    # A broad seamless studio floor; all lighting is generated locally.
    floor_mat = mat("Studio / midnight slate", (0.026, 0.035, 0.044, 1), roughness=0.52)
    bpy.ops.mesh.primitive_plane_add(size=10000, location=(0, 0, -1.015))
    floor = studio_object(bpy.context.object, stage)
    floor.name = "Studio / seamless charcoal floor"
    floor.data.materials.append(floor_mat)
    world = bpy.data.worlds.new("Studio / low neutral ambient")
    scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.19, 0.23, 0.28, 1)
    background.inputs["Strength"].default_value = 0.22
    area_light("Softbox / warm key", (-110, -60, 180), (0, 0, 5), 1_150_000, 150, (1.0, 0.91, 0.80), stage, 1.25)
    area_light("Strip / cool rim", (105, 80, 125), (0, 15, 20), 1_350_000, 110, (0.74, 0.85, 1.0), stage, 0.28)
    area_light("Softbox / front fill", (70, -150, 75), (0, 0, 15), 330_000, 100, (0.86, 0.92, 1.0), stage, 0.75)
    area_light("Strip / metal edge", (-25, 125, 155), (12, 35, 25), 650_000, 100, (1.0, 0.97, 0.89), stage, 0.18)
    hero = camera("01 / SQUARE / 1 to 1", (145, -191, 232), (0, 0, 13), stage, lens=66)
    hero.data.shift_x, hero.data.shift_y = -0.02, -0.05
    wide = camera("02 / WIDESCREEN / 16 to 9", (177, -205, 184), (0, 0, 13), stage, lens=55)
    wide.data.shift_x, wide.data.shift_y = -0.025, -0.045
    scene.camera = hero
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 128
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.025
    scene.cycles.use_denoising = True
    scene.cycles.max_bounces = 8
    scene.cycles.transparent_max_bounces = 4
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.exposure = -1.15
    # The .blend opens directly in the composed camera view.
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                area.spaces.active.region_3d.view_perspective = "CAMERA"
                area.spaces.active.clip_end = 10000
                area.spaces.active.shading.type = "MATERIAL"
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.update()
    assert len(cap_objects) == 9 and all(n in bpy.data.objects for n in (hero.name, wide.name))
    body = next(o for o in root.all_objects if o.name.startswith("Enclosure"))
    assert all(abs(a-b) < 0.001 for a,b in zip(body.dimensions, (85,120,22)))
    return scene, model, [(hero, "Tacpad_9_Primadon_1x1", 2048, 2048),
                          (wide, "Tacpad_9_Primadon_16x9", 2560, 1440)]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    scene, model, shots = build_studio()
    scene.render.resolution_x, scene.render.resolution_y = shots[0][2:]
    scene.render.resolution_percentage = 100
    scene.render.filepath = str(OUT / (shots[0][1]+".png"))
    for filename in ("tacpad9_blender.py", "tacpad9_render.py"):
        text = bpy.data.texts.load(str(HERE / filename))
        text.use_fake_user = True
    bpy.ops.file.pack_all()
    bpy.context.preferences.filepaths.save_version = 0
    project = OUT / "Tacpad_9_Primadon.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(project))
    if SETUP_ONLY:
        return
    folder = Path(tempfile.gettempdir()) / "tacpad9-primadon-preview" if PREVIEW else OUT
    folder.mkdir(exist_ok=True)
    for cam, name, width, height in shots:
        scene.camera = cam
        scene.render.resolution_x, scene.render.resolution_y = width, height
        scene.render.resolution_percentage = 45 if PREVIEW else 100
        scene.cycles.samples = 32 if PREVIEW else 128
        scene.render.filepath = str(folder / (name+".png"))
        print("TACPAD PHOTO:", name, flush=True)
        bpy.ops.render.render(write_still=True)
    if not PREVIEW:
        model["export_model"](str(OUT / "exports"), stl=True, obj=True)
        scene.camera = shots[0][0]
        scene.render.resolution_x, scene.render.resolution_y = shots[0][2:]
        scene.render.filepath = str(OUT / (shots[0][1]+".png"))
        bpy.ops.wm.save_as_mainfile(filepath=str(project))
    print("TACPAD STUDIO DONE:", project, flush=True)


if __name__ == "__main__":
    main()
