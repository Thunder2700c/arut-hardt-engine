plugins { id("com.android.application") version "8.7.3" }
android {
 namespace = "com.arut.engine"
 compileSdk = 35
 ndkVersion = "27.2.12479018"
 defaultConfig {
  applicationId = "com.arut.engine"
  minSdk = 26
  targetSdk = 35
  versionCode = 1
  versionName = "0.1.0"
  ndk { abiFilters += listOf("arm64-v8a") }
  externalNativeBuild {
   cmake {
    arguments += listOf("-DANDROID_STL=c++_shared", "-DARUT_BUILD_TESTS=OFF", "-DARUT_BUILD_RUNTIME=OFF")
    cppFlags += listOf("-std=c++20")
    targets += listOf("arut_android")
   }
  }
 }
 externalNativeBuild { cmake { path = file("../CMakeLists.txt"); version = "3.22.1" } }
 buildTypes {
  release { isMinifyEnabled = false }
  debug { applicationIdSuffix = ".debug"; versionNameSuffix = "-debug" }
 }
}
