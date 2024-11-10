add_subdirectory(Cantonese-IME)

add_library(cantonese SHARED cantonese.cpp)
set_property(TARGET cantonese PROPERTY CXX_STANDARD 23)
target_link_libraries(cantonese Fcitx5::Core)
target_link_libraries(cantonese ime)

install(TARGETS cantonese DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")


# Addon config file
# We need additional layer of conversion because we want PROJECT_VERSION in it.
configure_file(cantonese-addon.conf.in cantonese-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cantonese-addon.conf" RENAME cantonese.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input Method registration file
configure_file(cantonese.conf.in cantonese.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cantonese.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
