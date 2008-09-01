dnl Perform a check for a GStreamer element using gst-inspect
dnl Thomas Vander Stichele <thomas at apestaart dot org>
dnl Last modification: 23/10/2002

dnl AM_GST_ELEMENT_CHECK(ELEMENT-NAME, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)

AC_DEFUN([AM_GST_ELEMENT_CHECK],
[
  AC_CHECK_PROG(GST_INSPECT, gst-inspect-0.10, gst-inspect-0.10, [])
  if test "x$GST_INSPECT" == "x"; then
	  AC_CHECK_PROG(GST_INSPECT, gst-inspect-i386-0.10, gst-inspect-i386-0.10, [])
  fi
  if test "x$GST_INSPECT" != "x"; then
    AC_MSG_CHECKING(GStreamer element $1)
    if [ $GST_INSPECT $1 > /dev/null 2> /dev/null ]; then
      AC_MSG_RESULT(found.)
      $2
    else
      AC_MSG_RESULT(not found.)
      $3
    fi
  else
    AC_MSG_ERROR([gst-inspect-0.10 not found])
  fi
])

