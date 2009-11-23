# -*- Makefile -*-

AUTOMAKE_OPTIONS = foreign subdir-objects
SUBDIRS          = bin data doc kbd libexec

tmpdir := $(shell mktemp -dtu)
packer  = xz
packext = .tar.xz

.PHONY: tarball
tarball:
# do not use mkdir_p here.
	mkdir ${tmpdir}
	pushd ${top_srcdir} && git archive --prefix=${PACKAGE_NAME}-${PACKAGE_VERSION}/ HEAD | tar -C ${tmpdir} -x && popd;
	pushd ${tmpdir}/${PACKAGE_NAME}-${PACKAGE_VERSION} && ./autogen.sh && popd;
	tar --use=${packer} -C ${tmpdir} -cf ${PACKAGE_NAME}-${PACKAGE_VERSION}${packext} --owner=root --group=root ${PACKAGE_NAME}-${PACKAGE_VERSION}/;
	rm -Rf ${tmpdir};
