#!/bin/sh

if [ ! -d coverage ]; then
    mkdir coverage
fi
cd coverage

for i in `find .. -name "*.bb" -maxdepth 1`; do
    gcov $i -o ..
done

# It would be really nice to find a better way to do this than copying the 
# HTML into this script.  But, I am being lazy right now.
cat > index.html << EOF
<!-- This is a generated file, do not edit -->
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1"/>
    <meta name="author" content="APR Developers" /><meta name="email" content="dev@apr.apache.org" />
    <title>Test Coverage</title>
  </head>
  <body bgcolor="#ffffff" text="#000000" link="#525D76">
<p><a href="/"><img src="./images/apr_logo_wide.png" alt="The Apache Portable Runtime Project" border="0"/></a></p>
 <table border="0" width="100%" cellspacing="4">
  <tr>
   <!-- LEFT SIDE NAVIGATION -->
   <td valign="top" nowrap="nowrap">
    <a href="http://apachecon.com/"
    ><img src="http://www.apache.org/images/ac2003-150.gif" height="86"
    width="150" border="0" alt="ApacheCon" /></a>
          <p><b>Get Involved</b></p>
   <menu compact="compact">
         <li><a href="anoncvs.txt">CVS</a></li>
         <li><a href="mailing-lists.html">Mailing Lists</a></li>
         <li><a href="http://cvs.apache.org/snapshots/apr/">Snapshots</a></li>
         <li><a href="compiling_win32.html">Build on Win32</a></li>
         <li><a href="compiling_unix.html">Build on Unix</a></li>
       </menu>
     <p><b>Download!</b></p>
   <menu compact="compact">
         <li><a href="http://www.apache.org/dyn/closer.cgi/apr/">from a mirror</a></li>
       </menu>
     <p><b>Docs</b></p>
   <menu compact="compact">
         <li><a href="docs/apr/">APR</a></li>
         <li><a href="docs/apr-util/">APR-util</a></li>
         <li>APR-iconv</li>
       </menu>
     <p><b>Guidelines</b></p>
   <menu compact="compact">
         <li><a href="guidelines.html">Project Guidelines</a></li>
         <li><a href="patches.html">Contributing</a></li>
        <li><a href="versioning.html">Version Numbers</a></li>
      </menu>
    <p><b><a href="/info/">Miscellaneous</a></b></p>
  <menu compact="compact">
        <li><a href="http://www.apache.org/LICENSE.txt">License</a></li>
        <li><a href="projects.html">Projects using APR</a></li>
      </menu>
  </td>
  <!-- RIGHT SIDE INFORMATION -->
  <td align="left" valign="top">
  <table border="0" cellspacing="0" cellpadding="2" width="100%">
  <tr><td bgcolor="#525D76">
   <font color="#ffffff" face="arial,helvetica,sanserif">
    <strong>APR Test Coverage</strong>
   </font>
  </td></tr>
  <tr><td>
   <blockquote>
<p>This should give us some idea of how well our tests actually stress our
code.</p>
   </blockquote>

EOF

for i in `find . -name "*.gcov" -print`; do
    name=`echo $i | awk -F'/' {'print $2'}`
    echo " <a href=\"$name\">$name</a><br>"  >> index.html
done;

cat >> index.html << EOF
</td></tr>
</table>
   <!-- FOOTER -->
   <tr><td colspan="2"><hr noshade="noshade" size="1"/></td></tr>
   <tr><td colspan="2" align="center">
        <font size="-1">
         <em>Copyright &#169; 1999-2003, The Apache Software Foundation</em>
        </font>
       </td>
   </tr>
  </table>
 </body>
</html>

EOF
