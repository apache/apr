#ifndef APR_COMPAT_H
#define APR_COMPAT_H

/* redefine 1.3.x symbols to those that now live in libapr */

#define ap_inline apr_inline

#define ap_md5_ctx_t apr_md5_ctx_t
#define ap_MD5Encode apr_md5_encode
#define ap_MD5Final apr_md5_final
#define ap_MD5Init apr_md5_init
#define ap_MD5Update apr_md5_update
#define ap_append_arrays apr_array_append
#define ap_array_cat apr_array_cat
#define ap_array_header_t apr_array_header_t
#define ap_array_pstrcat apr_array_pstrcat
#define ap_bytes_in_free_blocks apr_pool_free_blocks_num_bytes
#define ap_bytes_in_pool apr_pool_num_bytes
#define ap_check_file_time apr_check_file_time
#define ap_filetype_e apr_filetype_e
#define ap_cleanup_for_exec apr_pool_cleanup_for_exec
#define ap_clear_pool apr_pool_clear
#define ap_clear_table apr_table_clear
#define ap_copy_array apr_array_copy
#define ap_copy_array_hdr apr_array_copy_hdr
#define ap_copy_table apr_table_copy
#define ap_cpystrn apr_cpystrn
#define ap_day_snames apr_day_snames
#define ap_destroy_pool apr_pool_destroy
#define ap_exploded_time_t apr_exploded_time_t
#define ap_fnmatch apr_fnmatch
#define ap_getopt apr_getopt
#define ap_inet_addr apr_inet_addr
#define ap_init_alloc apr_pool_alloc_init
#define ap_is_empty_table apr_is_empty_table
#define ap_is_fnmatch apr_is_fnmatch
#define ap_kill_cleanup apr_pool_cleanup_kill
#define ap_make_array apr_array_make
#define ap_make_sub_pool apr_pool_sub_make
#define ap_make_table apr_table_make
#define ap_month_snames apr_month_snames
#define ap_note_subprocess apr_pool_note_subprocess
#define ap_null_cleanup apr_pool_cleanup_null
#define ap_os_dso_load apr_dso_load
#define ap_os_dso_unload apr_dso_unload
#define ap_os_dso_sym apr_dso_sym
#define ap_os_dso_error apr_dso_error
#define ap_os_kill apr_proc_kill
#define ap_overlap_tables apr_table_overlap
#define ap_overlay_tables apr_table_overlay
#define ap_palloc apr_palloc
#define ap_pcalloc apr_pcalloc
#define ap_pool_join apr_pool_join
#define ap_psprintf apr_psprintf
#define ap_pstrcat apr_pstrcat
#define ap_pstrdup apr_pstrdup
#define ap_pstrndup apr_pstrndup
#define ap_push_array apr_array_push
#define ap_pvsprintf apr_pvsprintf
#define ap_register_cleanup apr_pool_cleanup_register
#define ap_register_other_child apr_proc_other_child_register
#define ap_run_cleanup apr_pool_cleanup_run
#define ap_signal apr_signal
#define ap_snprintf apr_snprintf
#define ap_table_add apr_table_add
#define ap_table_addn apr_table_addn
#define ap_table_do apr_table_do
#define ap_table_elts apr_table_elts
#define ap_table_get apr_table_get
#define ap_table_merge apr_table_merge
#define ap_table_mergen apr_table_mergen
#define ap_table_set apr_table_set
#define ap_table_setn apr_table_setn
#define ap_table_unset apr_table_unset
#define ap_unregister_other_child apr_proc_other_child_unregister
#define ap_validate_password apr_password_validate
#define ap_vformatter apr_vformatter
#define ap_vsnprintf apr_vsnprintf
#define ap_wait_t apr_wait_t

#define ap_isalnum apr_isalnum
#define ap_isalpha apr_isalpha
#define ap_iscntrl apr_iscntrl
#define ap_isdigit apr_isdigit
#define ap_isgraph apr_isgraph
#define ap_islower apr_islower
#define ap_isascii apr_isascii
#define ap_isprint apr_isprint
#define ap_ispunct apr_ispunct
#define ap_isspace apr_isspace
#define ap_isupper apr_isupper
#define ap_isxdigit apr_isxdigit
#define ap_tolower apr_tolower
#define ap_toupper apr_toupper

#define AP_USEC_PER_SEC APR_USEC_PER_SEC
#define AP_RFC822_DATE_LEN APR_RFC822_DATE_LEN
#define AP_OVERLAP_TABLES_MERGE APR_OVERLAP_TABLES_MERGE
#define AP_OVERLAP_TABLES_SET APR_OVERLAP_TABLES_SET

#endif /* APR_COMPAT_H */
