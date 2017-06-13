import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as Application

Project {
    name: "luam_gui_proj"
    references: [
        "luam_gui.qbs",
        "libs/UniversalModels/UniversalModels.qbs",
        "libs/Logger/logger.qbs",
    ]
}
