#ifndef APR_COMPAT_H
#define APR_COMPAT_H

/* redefine 1.3.x symbols to those that now live in libapr */

#define ap_MD5Encode apr_MD5Encode
#define ap_MD5Final apr_MD5Final
#define ap_MD5Init apr_MD5Init
#define ap_MD5Update apr_MD5Update
#define ap_append_arrays apr_append_arrays
#define ap_array_cat apr_array_cat
#define ap_array_pstrcat apr_array_pstrcat
#define ap_bytes_in_free_blocks apr_bytes_in_free_blocks
#define ap_bytes_in_pool apr_bytes_in_pool
#define ap_cleanup_for_exec apr_cleanup_for_exec
#define ap_clear_pool apr_clear_pool
#define ap_clear_table apr_clear_table
#define ap_copy_array apr_copy_array
#define ap_copy_array_hdr apr_copy_array_hdr
#define ap_copy_table apr_copy_table
#define ap_cpystrn apr_cpystrn
#define ap_destroy_pool apr_destroy_pool
#define ap_fnmatch apr_fnmatch
#define ap_init_alloc apr_init_alloc
#define ap_is_fnmatch apr_is_fnmatch
#define ap_kill_cleanup apr_kill_cleanup
#define ap_make_array apr_make_array
#define ap_make_sub_pool apr_make_sub_pool
#define ap_make_table apr_make_table
#define ap_note_subprocess apr_note_subprocess
#define ap_null_cleanup apr_null_cleanup
#define ap_overlap_tables apr_overlap_tables
#define ap_overlay_tables apr_overlay_tables
#define ap_palloc apr_palloc
#define ap_pcalloc apr_pcalloc
#define ap_psprintf apr_psprintf
#define ap_pstrcat apr_pstrcat
#define ap_pstrdup apr_pstrdup
#define ap_pstrndup apr_pstrndup
#define ap_push_array apr_push_array
#define ap_pvsprintf apr_pvsprintf
#define ap_register_cleanup apr_register_cleanup
#define ap_register_other_child apr_register_other_child
#define ap_run_cleanup apr_run_cleanup
#define ap_signal apr_signal
#define ap_snprintf apr_snprintf
#define ap_table_add apr_table_add
#define ap_table_addn apr_table_addn
#define ap_table_do apr_table_do
#define ap_table_get apr_table_get
#define ap_table_merge apr_table_merge
#define ap_table_mergen apr_table_mergen
#define ap_table_set apr_table_set
#define ap_table_setn apr_table_setn
#define ap_table_unset apr_table_unset
#define ap_unregister_other_child apr_unregister_other_child
#define ap_validate_password apr_validate_password
#define ap_vformatter apr_vformatter
#define ap_vsnprintf apr_vsnprintf

#endif /* APR_COMPAT_H */
