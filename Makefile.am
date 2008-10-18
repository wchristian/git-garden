# -*- Makefile -*-

AUTOMAKE_OPTIONS = foreign subdir-objects
SUBDIRS          = bin kbd libexec

.PHONY: tarball
tarball:
	rm -Rf /tmp/${PACKAGE_NAME}-${PACKAGE_VERSION};
	pushd ${top_srcdir} && git archive --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD | tar -C /tmp -x && popd;
	pushd /tmp/${PACKAGE_NAME}-${PACKAGE_VERSION} && ./autogen.sh && popd;
	tar --use=lzma -C /tmp -cf ${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.lzma --owner=root --group=root ${PACKAGE_NAME}-${PACKAGE_VERSION}/;
	rm -Rf /tmp/${PACKAGE_NAME}-${PACKAGE_VERSION};
