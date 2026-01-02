#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x8471422e, "inode_init_owner" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x5657e2ef, "d_instantiate" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0x81cc768b, "new_inode" },
	{ 0xb03fd70b, "unregister_filesystem" },
	{ 0x0d4cb875, "simple_statfs" },
	{ 0x692d8520, "d_make_root" },
	{ 0x5a844b26, "__x86_indirect_thunk_r15" },
	{ 0xf8491da5, "d_splice_alias" },
	{ 0x84e380da, "current_time" },
	{ 0xd36b6cbe, "iput" },
	{ 0xb03fd70b, "register_filesystem" },
	{ 0xd272d446, "__fentry__" },
	{ 0x1c434a31, "simple_inode_init_ts" },
	{ 0x5a844b26, "__x86_indirect_thunk_rax" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x2a80871a, "make_kuid" },
	{ 0x9479a1e8, "strnlen" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xd7a59a65, "vmalloc_noprof" },
	{ 0x2e24a63e, "set_nlink" },
	{ 0x2435d559, "strncmp" },
	{ 0xefc0bd20, "from_kgid" },
	{ 0xc609ff70, "strncpy" },
	{ 0xe54e0a6b, "__fortify_panic" },
	{ 0x81d6dd8f, "default_llseek" },
	{ 0x7883379a, "from_kuid" },
	{ 0x27683a56, "memset" },
	{ 0x5a844b26, "__x86_indirect_thunk_r10" },
	{ 0x47dc1b10, "__insert_inode_hash" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x1f291fe7, "make_kgid" },
	{ 0xc1020770, "mount_nodev" },
	{ 0xf1de9e85, "vfree" },
	{ 0xd0787cba, "generic_delete_inode" },
	{ 0xdae2bba8, "generic_file_open" },
	{ 0xebc4978b, "d_parent_ino" },
	{ 0x6907ad13, "__mark_inode_dirty" },
	{ 0x81d6dd8f, "generic_file_llseek" },
	{ 0xf360faf3, "inode_set_ctime_to_ts" },
	{ 0x86c49e96, "nop_mnt_idmap" },
	{ 0xba157484, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x8471422e,
	0xa61fd7aa,
	0x5657e2ef,
	0x092a35a2,
	0x81cc768b,
	0xb03fd70b,
	0x0d4cb875,
	0x692d8520,
	0x5a844b26,
	0xf8491da5,
	0x84e380da,
	0xd36b6cbe,
	0xb03fd70b,
	0xd272d446,
	0x1c434a31,
	0x5a844b26,
	0xe8213e80,
	0xd272d446,
	0x2a80871a,
	0x9479a1e8,
	0x90a48d82,
	0xd7a59a65,
	0x2e24a63e,
	0x2435d559,
	0xefc0bd20,
	0xc609ff70,
	0xe54e0a6b,
	0x81d6dd8f,
	0x7883379a,
	0x27683a56,
	0x5a844b26,
	0x47dc1b10,
	0xd272d446,
	0x092a35a2,
	0x1f291fe7,
	0xc1020770,
	0xf1de9e85,
	0xd0787cba,
	0xdae2bba8,
	0xebc4978b,
	0x6907ad13,
	0x81d6dd8f,
	0xf360faf3,
	0x86c49e96,
	0xba157484,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"inode_init_owner\0"
	"__check_object_size\0"
	"d_instantiate\0"
	"_copy_from_user\0"
	"new_inode\0"
	"unregister_filesystem\0"
	"simple_statfs\0"
	"d_make_root\0"
	"__x86_indirect_thunk_r15\0"
	"d_splice_alias\0"
	"current_time\0"
	"iput\0"
	"register_filesystem\0"
	"__fentry__\0"
	"simple_inode_init_ts\0"
	"__x86_indirect_thunk_rax\0"
	"_printk\0"
	"__stack_chk_fail\0"
	"make_kuid\0"
	"strnlen\0"
	"__ubsan_handle_out_of_bounds\0"
	"vmalloc_noprof\0"
	"set_nlink\0"
	"strncmp\0"
	"from_kgid\0"
	"strncpy\0"
	"__fortify_panic\0"
	"default_llseek\0"
	"from_kuid\0"
	"memset\0"
	"__x86_indirect_thunk_r10\0"
	"__insert_inode_hash\0"
	"__x86_return_thunk\0"
	"_copy_to_user\0"
	"make_kgid\0"
	"mount_nodev\0"
	"vfree\0"
	"generic_delete_inode\0"
	"generic_file_open\0"
	"d_parent_ino\0"
	"__mark_inode_dirty\0"
	"generic_file_llseek\0"
	"inode_set_ctime_to_ts\0"
	"nop_mnt_idmap\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "1294D975EE04962784C1229");
