import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
name: "luam_gui"
consoleApplication:false
type:"application"
qbsSearchPaths: sourceDirectory + "/modules"
Depends { name: "Qt.core"}
Depends { name: "Qt.sql" }
Depends { name: "Qt.core" }
Depends { name: "Qt.widgets" }
Depends { name: "Qt.network" }
Depends { name: "Qt.gui" }
Depends { name: "Qt.concurrent" }
Depends { name: "cpp" }
Depends { name: "UniversalModels" }
Depends { name: "logger" }

cpp.defines: base.concat(["L_TREE_CONTROLLER_LIBRARY", "L_LOGGER_LIBRARY"])
cpp.includePaths: [
                sourceDirectory,
                sourceDirectory + "/include",
                sourceDirectory + "/libs",
                sourceDirectory + "/libs/Logger/include",
]

files: [
        "include/init_database.h",
        "luamwindow.cpp",
        "luamwindow.h",
        "luamwindow.ui",
        "main.cpp",
        "sqlite/sqlite3.c",
        "sqlite/sqlite3.h",
        "src/init_database.cpp",
    ]

cpp.staticLibraries: ["UniversalModels", "logger"]
}
