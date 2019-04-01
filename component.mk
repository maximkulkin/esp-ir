INC_DIRS += $(ir_ROOT)

ir_INC_DIR = $(ir_ROOT)
ir_SRC_DIR = $(ir_ROOT)/ir

$(eval $(call component_compile_rules,ir))
