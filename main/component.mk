#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

$(info "UBIRCH Specific Component Makefile Process")

CPPFLAGS = -D__ESP32__


PACKAGE_DIR = ../../

COMPONENT_ADD_INCLUDEDIRS := ${PACKAGE_DIR}ubirch-mbed-msgpack \
                             ${PACKAGE_DIR}ubirch-mbed-msgpack/msgpack \
                             ${PACKAGE_DIR}ubirch-protocol/ubirch \
                             ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl \
                             ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/crypto_sign \
                             ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/include \
                             ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/randombytes \
                             ${PACKAGE_DIR}ubirch-protocol/ubirch/digest \
                             ${PACKAGE_DIR}esp-idf-v3.0.2/components/soc

COMPONENT_SRCDIRS := .  ${PACKAGE_DIR}ubirch-mbed-msgpack \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/crypto_hash \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/crypto_hashblocks \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/crypto_sign \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/crypto_verify \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/nacl/shared \
                        ${PACKAGE_DIR}ubirch-mbed-nacl-cm0/source/randombytes \
                        ${PACKAGE_DIR}ubirch-protocol/ubirch \
                        ${PACKAGE_DIR}ubirch-protocol/ubirch/digest

# embed files from the "certs" directory as binary data symbols
# in the app
# COMPONENT_EMBED_TXTFILES := TestCA_crt.pem
# COMPONENT_EMBED_TXTFILES := ubirchdemoubirchcom_crt.pem