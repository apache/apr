#!/usr/bin/perl -w
use strict;
use ExtUtils::MakeMaker qw(prompt);
use File::Find;

my $just_check = @ARGV ? $ARGV[0] eq '-c' : 0;
shift if $just_check;
my $dir = shift || '.';
my %names;

my $prefix = 'apr_';

while (<DATA>) {
    chomp;
    my($old, $new) = grep { s/^$prefix//o } split;
    next unless $old and $new;
    $names{$old} = $new;
}

my $pattern = join '|', keys %names;
#print "replacement pattern=$pattern\n";

find sub {
    chomp;
    return unless /\.[ch]$/;
    my $file = "$File::Find::dir/$_";
    print "looking in $file\n";

    replace($_, !$just_check);

}, $dir;

sub replace {
    my($file, $replace) = @_;
    local *IN, *OUT;
    my @lines;
    my $found = 0;

    open IN, $file or die "open $file: $!";

    while (<IN>) {
        for (m/[^_\"]*$prefix($pattern)\b/og) {
            $found++;
            print "   $file:$. apr_$_ -> apr_$names{$_}\n";
        }
        push @lines, $_ if $replace;
    }

    close IN;

    return unless $found and $replace;

#    my $ans = prompt("replace?", 'y');
#    return unless $ans =~ /^y/i;

    open OUT, ">$file" or die "open $file: $!";

    for (@lines) {
        unless (/^\#include/) {
            s/([^_\"]*$prefix)($pattern)\b/$1$names{$2}/og;
        }
        print OUT $_;
    }

    close OUT;
}

__DATA__
apr_pollfd_t:
apr_add_poll_socket          apr_poll_socket_add
apr_clear_poll_sockets       apr_poll_socket_clear
apr_get_polldata             apr_poll_data_get
apr_get_revents              apr_poll_revents_get
apr_mask_poll_socket         apr_poll_socket_mask
apr_remove_poll_socket       apr_poll_socket_remove
apr_set_polldata             apr_poll_data_set
apr_setup_poll               apr_poll_setup

apr_time_t:
apr_now                      apr_time_now
apr_implode_gmt              apr_time_exp_gmt_get

apr_array_header_t:
apr_append_arrays            apr_array_append
apr_copy_array               apr_array_copy
apr_copy_array_hdr           apr_array_copy_hdr
apr_make_array               apr_array_make
apr_push_array               apr_array_push

apr_socket_t:
apr_close_socket             apr_socket_close
apr_create_socket            apr_socket_create
apr_get_sockaddr             apr_socket_addr_get
apr_get_socketdata           apr_socket_data_get
apr_set_socketdata           apr_socket_data_set
apr_shutdown                 apr_socket_shutdown
apr_bind                     apr_socket_bind
apr_listen                   apr_socket_listen
apr_accept                   apr_socket_accept
apr_connect                  apr_socket_connect
apr_send                     apr_socket_send
apr_sendv                    apr_socket_sendv
apr_sendto                   apr_socket_sendto
apr_recvfrom                 apr_socket_recvfrom
apr_sendfile                 apr_socket_sendfile
apr_recv                     apr_socket_recv



apr_sockaddr_t:
apr_getaddrinfo              apr_sockaddr_info_get
apr_get_ipaddr               apr_sockaddr_ip_get
apr_set_ipaddr               apr_sockaddr_ip_set
apr_set_port                 apr_sockaddr_port_set
apr_get_port                 apr_sockaddr_port_get

apr_pool_t:
apr_create_pool              apr_pool_create
apr_destroy_pool             apr_pool_destroy
apr_get_userdata             apr_pool_userdata_get
apr_set_userdata             apr_pool_userdata_set
apr_kill_cleanup             apr_pool_cleanup_kill
apr_run_cleanup              apr_pool_cleanup_run
apr_null_cleanup             apr_pool_cleanup_null
apr_register_cleanup         apr_pool_cleanup_register
apr_make_sub_pool            apr_pool_sub_make
apr_note_subprocess          apr_pool_note_subprocess
apr_bytes_in_pool            apr_pool_num_bytes
apr_bytes_in_free_blocks     apr_pool_free_blocks_num_bytes
apr_cleanup_for_exec         apr_pool_cleanup_for_exec
apr_init_alloc               apr_pool_alloc_init
apr_term_alloc               apr_pool_alloc_term

apr_lock_t:
apr_child_init_lock          apr_lock_child_init
apr_create_lock              apr_lock_create
apr_destroy_lock             apr_lock_destroy
apr_get_lockdata             apr_lock_data_get
apr_set_lockdata             apr_lock_data_set
apr_lock                     apr_lock_aquire
apr_unlock                   apr_lock_release

apr_table_:
apr_clear_table              apr_table_clear
apr_copy_table               apr_table_copy
apr_make_table               apr_table_make
apr_overlap_tables           apr_table_overlap
apr_overlay_tables           apr_table_overlay

apr_file_t:
apr_open                     apr_file_open
apr_close                    apr_file_close
apr_create_namedpipe         apr_file_namedpipe_create
apr_create_pipe              apr_file_pipe_create
apr_dupfile                  apr_file_dup
apr_flush                    apr_file_flush
apr_eof                      apr_file_eof
apr_ferror                   apr_file_error
apr_fgets                    apr_file_gets
apr_fprintf                  apr_file_printf
apr_full_read                apr_file_read_file
apr_full_write               apr_file_write_full
apr_getc                     apr_file_getc
apr_ungetc                   apr_file_ungetc
apr_putc                     apr_file_putc
apr_puts                     apr_file_puts
apr_read                     apr_file_read
apr_write                    apr_file_write
apr_writev                   apr_file_writev
apr_seek                     apr_file_seek
apr_get_filedata             apr_file_data_get
apr_getfileinfo              apr_file_info_get
apr_get_filename             apr_file_name_get
apr_get_file_pool            apr_file_pool_get
apr_get_pipe_timeout         apr_file_pipe_timeout_get
apr_set_pipe_timeout         apr_file_pipe_timeout_set
apr_lock_file                apr_file_lock
apr_unlock_file              apr_file_unlock
apr_open_stderr              apr_file_open_stderr
apr_open_stdout              apr_file_open_stdout
apr_remove_file              apr_file_remove
apr_rename_file              apr_file_rename
apr_set_filedata             apr_file_data_set
apr_setfileperms             apr_file_perms_set

apr_filepath_*:
apr_filename_of_pathname     apr_filepath_name_get

apr_procattr_t:
apr_createprocattr_init      apr_procattr_create
apr_setprocattr_childerr     apr_procattr_child_err_set
apr_setprocattr_childin      apr_procattr_child_in_set
apr_setprocattr_childout     apr_procattr_child_out_set
apr_setprocattr_cmdtype      apr_procattr_cmdtype_set
apr_setprocattr_detach       apr_procattr_detach_set
apr_setprocattr_dir          apr_procattr_dir_set
apr_setprocattr_io           apr_procattr_io_set
apr_setprocattr_limit        apr_procattr_limit_set

apr_proc_t:
apr_create_process           apr_proc_create
apr_fork                     apr_proc_fork
apr_kill                     apr_proc_kill
apr_probe_writable_fds       apr_proc_probe_writable_fds
apr_reap_other_child         apr_proc_other_child_read
apr_register_other_child     apr_proc_other_child_register
apr_unregister_other_child   apr_proc_other_child_unregister
apr_check_other_child        apr_proc_other_child_check
apr_wait_all_procs           apr_proc_wait_all_procs
apr_wait_proc                apr_proc_wait
apr_detach                   apr_proc_detach

apr_thread_t:
apr_create_thread            apr_thread_create
apr_get_threaddata           apr_thread_data_get
apr_set_threaddata           apr_thread_data_set
apr_thread_detach            apr_thread_detach

apr_threadkey_t:
apr_get_threadkeydata        apr_threadkey_data_get
apr_set_threadkeydata        apr_threadkey_data_set
apr_create_thread_private    apr_threadkey_private_create
apr_delete_thread_private    apr_threadkey_private_delete
apr_get_thread_private       apr_threadkey_private_get
apr_set_thread_private       apr_threadkey_private_set

apr_threadatt_t:
apr_create_threadattr        apr_threadattr_create
apr_getthreadattr_detach     apr_threadattr_detach_set
apr_setthreadattr_detach     apr_threadattr_detach_get

apr_dir_t:
apr_make_dir                 apr_dir_make
apr_remove_dir               apr_dir_remove

apr_gid_t:
apr_get_groupid              apr_gid_get
apr_get_groupname            apr_gid_name_get
apr_group_name_get           apr_gid_name_get
apr_compare_groups           apr_gid_compare

apr_uuid_t:
apr_format_uuid              apr_uuid_format
apr_get_uuid                 apr_uuid_get
apr_parse_uuid               apr_uuid_parse

apr_uid_t:
apr_get_home_directory       apr_uid_homepath_get
apr_get_userid               apr_uid_get
apr_current_userid           apr_uid_current
apr_compare_users            apr_uid_compare
apr_get_username             apr_uid_name_get
apr_compare_users            apr_uid_compare

apr_shmem_t:
apr_get_shm_name             apr_shm_name_get
apr_set_shm_name             apr_shm_name_set
apr_open_shmem               apr_shm_open

apr_hash_t:
apr_make_hash                apr_hash_make
apr_getpass                  apr_password_get
apr_validate_password        apr_password_validate
apr_generic_hook_get         apr_hook_generic_get
apr_hook_generic             apr_hook_generic_add

apr_bucket_*:
apr_bucket_copy_notimpl      apr_bucket_notimpl_copy
apr_bucket_copy_shared       apr_bucket_shared_copy
apr_bucket_create_eos        apr_bucket_eos_create
apr_bucket_create_file       apr_bucket_file_create
apr_bucket_create_flush      apr_bucket_flush_create
apr_bucket_create_heap       apr_bucket_heap_create
apr_bucket_create_immortal   apr_bucket_immortal_create
apr_bucket_create_mmap       apr_bucket_mmap_create
apr_bucket_create_pipe       apr_bucket_pipe_creat
apr_bucket_create_pool       apr_bucket_pool_create
apr_bucket_create_socket     apr_bucket_socket_create
apr_bucket_create_transient  apr_bucket_transient_create
apr_bucket_destroy_notimpl   apr_bucket_notimpl_destroy
apr_bucket_destroy_shared    apr_bucket_shared_destroy
apr_bucket_make_eos          apr_bucket_eos_make
apr_bucket_make_file         apr_bucket_file_make
apr_bucket_make_flush        apr_bucket_flush_make
apr_bucket_make_heap         apr_bucket_heap_make
apr_bucket_make_immortal     apr_bucket_immortal_make
apr_bucket_make_mmap         apr_bucket_mmap_make
apr_bucket_make_pipe         apr_bucket_pipe_make
apr_bucket_make_pool         apr_bucket_pool_make
apr_bucket_make_shared       apr_bucket_shared_make
apr_bucket_make_socket       apr_bucket_socket_make
apr_bucket_make_transient    apr_bucket_transient_make
apr_bucket_setaside_notimpl  apr_bucket_notimpl_setaside
apr_bucket_split_notimpl     apr_bucket_notimpl_split
apr_bucket_split_shared      apr_bucket_shared_split
apr_init_bucket_types        apr_bucket_init_types
apr_insert_bucket_type       apr_bucket_insert_type

apr_os_*:
apr_get_os_dir               apr_os_dir_get
apr_get_os_exp_time          apr_os_exp_time_get
apr_get_os_file              apr_os_file_get
apr_get_os_imp_time          apr_os_imp_time_get
apr_get_os_lock              apr_os_lock_get
apr_get_os_sock              apr_os_sock_get
apr_get_os_thread            apr_os_thread_get
apr_get_os_threadkey         apr_os_threadkey_get
apr_make_os_sock             apr_os_sock_make
apr_put_os_dir               apr_os_dir_put
apr_put_os_exp_time          apr_os_exp_time_put
apr_put_os_file              apr_os_file_put
apr_put_os_imp_time          apr_os_imp_time_put
apr_put_os_lock              apr_os_lock_put
apr_put_os_sock              apr_os_sock_put
apr_put_os_thread            apr_os_thread_put
apr_put_os_threadkey         apr_os_threadkey_put

apr_md5_ctx_t:
apr_MD5Encode                apr_md5_encode
apr_MD5Final                 apr_md5_final
apr_MD5Init                  apr_md5_init
apr_MD5SetXlate              apr_md5_set_xlate
apr_MD5Update                apr_md5_update

apr_sha1_ctx_t:
apr_SHA1Final                apr_sha1_final
apr_SHA1Init                 apr_sha1_init
apr_SHA1Update               apr_sha1_update
apr_SHA1Update_binary        apr_sha1_update_binary

apr_getopt_t:
apr_initopt                  apr_getopt_init

apr_base64_*:
apr_base64decode             apr_base64_decode
apr_base64decode_binary      apr_base64_decode_binary
apr_base64decode_len         apr_base64_decode_len
apr_base64encode             apr_base64_encode
apr_base64encode_binary      apr_base64_encode_binary
apr_base64encode_len         apr_base64_encode_len
