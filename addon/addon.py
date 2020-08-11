bl_info = {
    "name": "Prism Asset Pipeline",
    "blender": (2, 80, 0),
    "location": "File > Import-Export",
    "category": "Import-Export"
}

import bpy
from bpy.types import Operator, AddonPreferences
from bpy.props import StringProperty, BoolProperty, PointerProperty
from bpy_extras.io_utils import (
        ExportHelper,
        )
import subprocess

class MarkExportOperator(bpy.types.Operator):
    bl_idname = "scene.mark_export_operator"
    bl_label = "Mark All Objects for Export"

    def execute(self, context):
        for ob in context.scene.objects:
            if ob.type == 'MESH':
                ob.willExport = True

        return {'FINISHED'}

class HelloWorldPanel(bpy.types.Panel):
    bl_idname = "OBJECT_PT_hello_world"
    bl_label = "Prism Asset Pipeline"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    def draw(self, context):
        self.layout.prop(context.object, "modelContainer", text="Model Group")

        self.layout.prop(context.object, "willExport", text="Will Export")
        self.layout.prop(context.object, "exportArmature", text="Export Armature")
        self.layout.prop(context.object, "exportAnim", text="Export Animations")

class ExportSettingsPanel(bpy.types.Panel):
    bl_idname = "OBJECT_PT_prism_srttings"
    bl_label = "Prism Asset Pipeline"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"

    def draw(self, context):
        self.layout.operator("scene.mark_export_operator")
        self.layout.prop(context.scene, "export_prefix", text="Export Prefix")
        self.layout.prop(context.scene, "removeTransforms", text="Remove Transforms")

def console_get():
    for area in bpy.context.screen.areas:
        if area.type == 'CONSOLE':
            for space in area.spaces:
                if space.type == 'CONSOLE':
                    return area, space
    return None, None

def console_write(text):
    area, space = console_get()
    if space is None:
        return

    context = bpy.context.copy()
    context.update(dict(
        space=space,
        area=area,
    ))
    for line in text.split("\n"):
        bpy.ops.console.scrollback_append(context, text=line, type='OUTPUT')

class ExampleAddonPreferences(AddonPreferences):
    bl_idname = __name__
    
    def update_func(self, context):
        if "//" in self.modelCompilerPath:
            self.modelCompilerPath = bpy.path.abspath(self.modelCompilerPath)
            
        if "//" in self.dataExportPath:
            self.dataExportPath = bpy.path.abspath(self.dataExportPath)

    modelCompilerPath = StringProperty(
            name="Model Compiler Path",
            subtype='FILE_PATH',
            update=update_func
            )
    dataExportPath = StringProperty(
            name="Data Export Path",
            subtype='DIR_PATH',
            update=update_func
            )

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "modelCompilerPath")
        layout.prop(self, "dataExportPath")
        

class ExportAssetPipeline(bpy.types.Operator):
    bl_idname = "prism.export_pipeline"
    bl_label = "Export Prism Engine"

    action = bpy.props.StringProperty(default="EVERYTHING")

    def execute(self, context):
        preferences = context.preferences
        addon_prefs = preferences.addons[__name__].preferences

        if len(addon_prefs.modelCompilerPath) == 0 or len(addon_prefs.dataExportPath) == 0:
            console_write("No modelS compiler or export path!")
            return {'FINISHED'}

        model_compiler = bpy.path.abspath(addon_prefs.modelCompilerPath)

        export_groups = {}

        for object in context.scene.objects:
            if object.willExport:
                if not object.modelContainer in export_groups:
                    export_groups[object.modelContainer] = {}
                    export_groups[object.modelContainer]["objects"] = []

                console_write("exporting " + object.name + " as " + object.modelContainer)

                export_groups[object.modelContainer]["objects"].append(object.name)

                if object.exportArmature is not None:
                    export_groups[object.modelContainer]["armature"] = object.exportArmature.name

        console_write(str(export_groups))

        for group in export_groups.keys():
            bpy.ops.object.select_all(action='DESELECT')

            will_export_anim = False

            if "armature" in export_groups[group]:
                object = context.scene.objects[export_groups[group]["armature"]]

                object.select_set(state=True)

            for object_name in export_groups[group]["objects"]:
                object = context.scene.objects[object_name]

                if object.exportAnim == True:
                    will_export_anim = True

                object.select_set(state=True)

            name_part = context.scene.export_prefix + group

            fbx_path = "//" + name_part + ".fbx"
            fbx_path = bpy.path.abspath(fbx_path)

            if self.action == 'ONLY_MESH':
                will_export_anim = False
                
            options_dict = {'filepath': fbx_path,
                            'check_existing': False,
                            'use_selection': True,
                            'bake_anim': will_export_anim,
                            'apply_unit_scale': True,
                            'add_leaf_bones': False,
                            'use_armature_deform_only': True,
                            'mesh_smooth_type': 'OFF',
                            'bake_space_transform': context.scene.removeTransforms,
                            'use_tspace': True
                }

            if context.scene.removeTransforms:
                #options_dict['bake_space_transform'] =True
                bpy.ops.export_scene.fbx(**options_dict)
            else:
                bpy.ops.export_scene.fbx(**options_dict)

            args = [model_compiler, "--no_ui", "--model-path", fbx_path, "--data-path", bpy.path.abspath(addon_prefs.dataExportPath)]

            if context.scene.removeTransforms:
                args.append("--compile-static")

            p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
            output, errors = p.communicate()

        return {'FINISHED'}

class PrismExportSubMenu(bpy.types.Menu):
    bl_idname = "OBJECT_MT_prism_export_submenu"
    bl_label = "Prism Asset Pipeline"

    def draw(self, context):
        self.layout.operator(ExportAssetPipeline.bl_idname, text="Everything").action = 'EVERYTHING'
        self.layout.operator(ExportAssetPipeline.bl_idname, text="Only Meshes").action = 'ONLY_MESH'

def menu_func_export(self, context):
    self.layout.menu(PrismExportSubMenu.bl_idname)

def register():
    bpy.utils.register_class(HelloWorldPanel)
    bpy.utils.register_class(ExportSettingsPanel)
    bpy.utils.register_class(ExportAssetPipeline)
    bpy.utils.register_class(MarkExportOperator)
    bpy.utils.register_class(PrismExportSubMenu)

    # properties
    bpy.types.Object.modelContainer = StringProperty()
    bpy.types.Object.willExport = BoolProperty()
    bpy.types.Object.exportAnim = BoolProperty()
    bpy.types.Scene.export_prefix = StringProperty()
    bpy.types.Scene.removeTransforms = BoolProperty()
    bpy.types.Object.exportArmature = PointerProperty(type=bpy.types.Object)

    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

    bpy.utils.register_class(ExampleAddonPreferences)

def unregister():
    bpy.utils.unregister_class(HelloWorldPanel)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    bpy.utils.unregister_class(ExportAssetPipeline)
    bpy.utils.unregister_class(ExampleAddonPreferences)
    bpy.utils.unregister_class(MarkExportOperator)

if __name__ == "__main__":
    register()
