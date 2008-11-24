# -*- Makefile -*-

AUTOMAKE_OPTIONS = foreign subdir-objects
SUBDIRS          = bin data doc kbd libexec

packer  = lzip
packext = .tar.lz

.PHONY: tarball
tarball:
	rm -Rf /tmp/${PACKAGE_NAME}-${PACKAGE_VERSION};
	pushd ${top_srcdir} && git archive --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD | tar -C /tmp -x && popd;
	pushd /tmp/${PACKAGE_NAME}-${PACKAGE_VERSION} && ./autogen.sh && popd;
	tar --use=${packer} -C /tmp -cf ${PACKAGE_NAME}-${PACKAGE_VERSION}${packext} --owner=root --group=root ${PACKAGE_NAME}-${PACKAGE_VERSION}/;
	rm -Rf /tmp/${PACKAGE_NAME}-${PACKAGE_VERSION};
