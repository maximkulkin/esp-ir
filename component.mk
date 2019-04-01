INC_DIRS += $(ir_i2s_ROOT)

ir_i2s_INC_DIR = $(ir_i2s_ROOT)
ir_i2s_SRC_DIR = $(ir_i2s_ROOT)/esp_ir

$(eval $(call component_compile_rules,ir_i2s))
