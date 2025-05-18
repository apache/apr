set(VERSION 2.0.0)
#
# Modify REF to latest commit id from https://github.com/apache/apr
# Update SHA512 with actual SHA512
#
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO apache/apr
    REF 6445e8804008922f8018aa238aa4d6bba608c49a
    SHA512 0
    HEAD_REF trunk
)

if (VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
        FEATURES
            private-headers INSTALL_PRIVATE_H
            crypto FEATURE_CRYPTO
            xlate FEATURE_XLATE
            dbd-odbc FEATURE_DBD_ODBC
            dbd-sqlite3 FEATURE_DBD_SQLITE3
            dbd-postgresql FEATURE_DBD_PGQL
            ldap FEATURE_LDAP
    )

    vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
            -DAPR_BUILD_TESTAPR=OFF
            -DINSTALL_PDB=OFF
            -DAPU_HAVE_CRYPTO=${FEATURE_CRYPTO}
            -DAPU_HAVE_ICONV=${FEATURE_XLATE}
            -DAPU_HAVE_ODBC=${FEATURE_DBD_ODBC}
            -DAPU_HAVE_SQLITE3=${FEATURE_DBD_SQLITE3}
            -DAPU_HAVE_PGSQL=${FEATURE_DBD_PGQL}
            -DAPR_HAS_LDAP=${FEATURE_LDAP}
            -DAPU_USE_EXPAT=${APU_USE_EXPAT}
            -DAPR_INSTALL_PRIVATE_H=${INSTALL_PRIVATE_H}
    )

    vcpkg_cmake_install()
    vcpkg_copy_pdbs()
    vcpkg_cmake_config_fixup(PACKAGE_NAME "apr"
                             CONFIG_PATH "lib/cmake/apr")

    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
else()
    # In development
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

