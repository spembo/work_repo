#!/bin/bash


DIR=$PWD
git_bin=$(which git)

mkdir -p "${DIR}/deploy/"



make_pkg () {
	cd "${DIR}/KERNEL" || exit

	deployfile="-${pkg}.tar.gz"
	tar_options="--create --gzip --file"

	if [ -f "${DIR}/deploy/${KERNEL_UTS}${deployfile}" ] ; then
		rm -rf "${DIR}/deploy/${KERNEL_UTS}${deployfile}" || true
	fi

	if [ -d "${DIR}/deploy/tmp" ] ; then
		rm -rf "${DIR}/deploy/tmp" || true
	fi
	mkdir -p "${DIR}/deploy/tmp"

	echo "-----------------------------"
	echo "Building ${pkg} archive..."

	case "${pkg}" in
	modules)
		make -s ARCH=${KERNEL_ARCH} LOCALVERSION=${BUILD} CROSS_COMPILE="${CC}" modules_install INSTALL_MOD_PATH="${DIR}/deploy/tmp"
		;;
	dtbs)
		make -s ARCH=${KERNEL_ARCH} LOCALVERSION=${BUILD} CROSS_COMPILE="${CC}" dtbs_install INSTALL_DTBS_PATH="${DIR}/deploy/tmp"
		;;
	esac

	echo "Compressing ${KERNEL_UTS}${deployfile}..."
	cd "${DIR}/deploy/tmp" || true
	tar ${tar_options} "../${KERNEL_UTS}${deployfile}" ./*

	cd "${DIR}/" || exit
	rm -rf "${DIR}/deploy/tmp" || true

	if [ ! -f "${DIR}/deploy/${KERNEL_UTS}${deployfile}" ] ; then
		export ERROR_MSG="File Generation Failure: [${KERNEL_UTS}${deployfile}]"
		/bin/sh -e "${DIR}/scripts/error.sh" && { exit 1 ; }
	else
		ls -lh "${DIR}/deploy/${KERNEL_UTS}${deployfile}"
	fi
}



make_dtbs_pkg () {
	pkg="dtbs"
	make_pkg
}




if [  -f "${DIR}/.yakbuild" ] ; then
	if [ -f "${DIR}/recipe.sh.sample" ] ; then
		if [ ! -f "${DIR}/recipe.sh" ] ; then
			cp -v "${DIR}/recipe.sh.sample" "${DIR}/recipe.sh"
		fi
	fi
fi

/bin/sh -e "${DIR}/tools/host_det.sh" || { exit 1 ; }

if [ ! -f "${DIR}/system.sh" ] ; then
	cp -v "${DIR}/system.sh.sample" "${DIR}/system.sh"
fi



unset CC
unset LINUX_GIT
. "${DIR}/system.sh"
if [  -f "${DIR}/.yakbuild" ] ; then
	. "${DIR}/recipe.sh"
fi
/bin/sh -e "${DIR}/scripts/gcc.sh" || { exit 1 ; }
. "${DIR}/.CC"
echo "CROSS_COMPILE=${CC}"
if [ -f /usr/bin/ccache ] ; then
	echo "ccache [enabled]"
	CC="ccache ${CC}"
fi

. "${DIR}/version.sh"
export LINUX_GIT

if [ ! "${CORES}" ] ; then
	CORES=$(getconf _NPROCESSORS_ONLN)
fi

unset FULL_REBUILD
#FULL_REBUILD=1
if [ "${FULL_REBUILD}" ] ; then
	/bin/sh -e "${DIR}/scripts/git.sh" || { exit 1 ; }

	if [ "${RUN_BISECT}" ] ; then
		/bin/sh -e "${DIR}/scripts/bisect.sh" || { exit 1 ; }
	fi

##	patch_kernel
##	copy_defconfig
fi


##if [ ! "${AUTO_BUILD}" ] ; then
##	make_menuconfig
##fi


if [  -f "${DIR}/.yakbuild" ] ; then
	BUILD=$(echo ${kernel_tag} | sed 's/[^-]*//'|| true)
fi


##make_kernel

if [ ! "${AUTO_BUILD_DONT_PKG}" ] ; then
##	make_modules_pkg
    echo "***** MAKING DTB ******"
	make_dtbs_pkg
fi


echo "-----------------------------"
echo "Script Complete"
echo "${KERNEL_UTS}" > kernel_version
echo "eewiki.net: [user@localhost:~$ export kernel_version=${KERNEL_UTS}]"
echo "-----------------------------"






