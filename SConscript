
Import("env")

files = env.core_lib_files()

(major, minor, patch) = env.APRVersion()

libapr = env.SharedLibrary('apr-%d' % (major), files)

targets = [libapr]

Return("targets")