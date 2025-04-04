// swift-tools-version:5.3
import PackageDescription

var sources = [
    "src/**/*.cpp"
]

var cxxSettings: [CXXSetting] = [
    // Use at least C++14 (needed for std::make_unique, etc.)
    .unsafeFlags(["-std=c++17", "-Wno-shorten-64-to-32", "-O3", "-DNDEBUG"]),
    .unsafeFlags(["-fno-objc-arc"]),
    .headerSearchPath("include"),
    .headerSearchPath("src"),
]

var cSettings: [CSetting] =  [
    .unsafeFlags(["-Wno-shorten-64-to-32", "-O3", "-DNDEBUG"]),
    .unsafeFlags(["-fno-objc-arc"]),
    .headerSearchPath("include"),
    .headerSearchPath("scr"),
]

let package = Package(
    name: "pqc",
    platforms: [
        .iOS(.v14),         // Specify iOS 16 or later; adjust as needed
        .macOS(.v10_15)
    ],
    products: [
        // Static library (akin to 'pqc' target)
        .library(
            name: "pqc",
            // type: .dynamic,
            targets: ["pqc"]
        )
//        // Optional: Shared library (akin to 'pqc_shared' target)
//        .library(
//            name: "tq42-pqc-oss-shared",
//            type: .dynamic,
//            targets: ["tq42-pqc-oss"]
//        )
    ],
    targets: [
        .target(
            name: "pqc",
            path: ".",                  // Source files are in 'src/'
            exclude: [
                "build",
                "benchmark",
                "examples",
                "docs",
                "python",
                "test",
                "CMakeLists.txt"
            ],
            publicHeadersPath: "include", // Headers are in 'include/' relative to 'src/'
            cSettings: cSettings,
            cxxSettings: cxxSettings
        )
    ]
)
