#
#
#  Ported from <http://www.scons.org/wiki/SubstInFileBuilder>
#  """ This code is freely available for your use. """
#

import re
from SCons.Script import * 

def do_subst_in_file(targetfile, sourcefile, dict):
    """Replace all instances of the keys of dict with their values.
    For example, if dict is {'%VERSION%': '1.2345', '%BASE%': 'MyProg'},
    then all instances of %VERSION% in the file will be replaced with 1.2345 etc.
    """
    try:
        f = open(sourcefile, 'rb')
        contents = f.read()
        f.close()
    except:
        raise SCons.Errors.UserError, "Can't read source file %s"%sourcefile
    for (k,v) in dict.items():
        contents = re.sub(k, v, contents)
    try:
        f = open(targetfile, 'wb')
        f.write(contents)
        f.close()
    except:
        raise SCons.Errors.UserError, "Can't write target file %s"%targetfile
    return 0 # success

def subst_in_file(target, source, env):
    if not env.has_key('SUBST_DICT'):
        raise SCons.Errors.UserError, "SubstFile requires SUBST_DICT to be set."
    d = dict(env['SUBST_DICT']) # copy it
    for (k,v) in d.items():
        if callable(v):
            d[k] = env.subst(v())
        elif SCons.Util.is_String(v):
            d[k]=env.subst(v)
        else:
            d[k] = SCons.Util.to_String(v)
    for (t,s) in zip(target, source):
        return do_subst_in_file(str(t), str(s), d)

def subst_in_file_string(target, source, env):
    """This is what gets printed on the console."""
    return '\n'.join(['Substituting vars from %s into %s'%(str(s), str(t))
                      for (t,s) in zip(target, source)])

def subst_emitter(target, source, env):
    """Add dependency from substituted SUBST_DICT to target.
    Returns original target, source tuple unchanged.
    """
    d = env['SUBST_DICT'].copy() # copy it
    for (k,v) in d.items():
        if callable(v):
            d[k] = env.subst(v())
        elif SCons.Util.is_String(v):
            d[k] = env.subst(v)
    Depends(target, SCons.Node.Python.Value(d))
    return target, source

def generate(env):
  subst_action=SCons.Action.Action(subst_in_file, subst_in_file_string)
  env['BUILDERS']['SubstFile'] = Builder(action=subst_action, emitter=subst_emitter)

def exists(env):
    return 1
