
Import("env")

files = env.core_lib_files()

(major, minor, patch) = env.APRVersion()

libapr = env.SharedLibrary('apr-%d' % (major), files)
tests = Split("""
  abts.c testutil.c
  testtime.c teststr.c testvsn.c testipsub.c testshm.c
  testmmap.c testud.c testtable.c testsleep.c testpools.c
	testfmt.c testfile.c testdir.c testfileinfo.c testrand.c
	testdso.c testoc.c testdup.c testsockets.c testproc.c
	testpoll.c testlock.c testsockopt.c testpipe.c
	testthread.c testhash.c testargs.c testnames.c testuser.c
	testpath.c testenv.c testprocmutex.c testfnmatch.c
	testatomic.c testflock.c testsock.c testglobalmutex.c
	teststrnatcmp.c testfilecopy.c testtemp.c testlfs.c
	testcond.c testuri.c testmemcache.c testdate.c
	testxlate.c testdbd.c testrmm.c testmd4.c
	teststrmatch.c testpass.c testcrypto.c testqueue.c
	testbuckets.c testxml.c testdbm.c testuuid.c testmd5.c
	testreslist.c dbd.c
""")

tenv = env.Clone()
tenv.AppendUnique(LIBS = libapr)
testall = tenv.Program('testall', source = ["test/"+t for t in tests])

targets = [libapr, testall]

Return("targets")
