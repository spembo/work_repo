#!/bin/bash

# SD Card format:
# BOOT:     MLO, u-boot.img
# ROOT_FS:  RFS, uEnv.txt, zImage, dtb, modules, file system table


KERNEL_VERSION=5.4.115-bone49
BBB_U_DIR=../

DISK=/dev/sdb
SD_CARD_PATH=media/${USER}
SD_CARD_ROOT_FS=${SD_CARD_PATH}/ROOT_FS
SD_CARD_BOOT=${SD_CARD_PATH}/BOOT

# Copy fail flags
FLAG_COPY_ROOT_FS=0
FLAG_SET_UNAME=0
FLAG_COPY_ZIMAGE=0
FLAG_COPY_DTB=0
FLAG_COPY_MODULES=0
FLAG_SET_FSTAB=0
FLAG_COPY_MLO=0
FLAG_COPY_UBOOT=0


# Copy Root File System
copy_root_fs() {
    echo "Copying root file system"

    if [[ -d /${SD_CARD_ROOT_FS} ]]; then
        sudo tar xfvp ${BBB_U_DIR}/*-*-*-armhf-*/armhf-rootfs-*.tar -C /${SD_CARD_ROOT_FS}/
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_COPY_ROOT_FS=1
        fi
    else
        FLAG_COPY_ROOT_FS=1
    fi
    
    sudo chown root:root /${SD_CARD_ROOT_FS}
    sudo chmod 755 /${SD_CARD_ROOT_FS}
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        FLAG_COPY_ROOT_FS=1
    fi
}


# Set uname_r in /boot/uEnv.txt
set_uname_in_uenv(){
    echo "Set uname_r in /boot/uEnv.txt"

    if [[ -d /${SD_CARD_ROOT_FS}/boot/ ]]; then
        sudo sh -c "echo 'uname_r=${KERNEL_VERSION}' >> /${SD_CARD_ROOT_FS}/boot/uEnv.txt"
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_SET_UNAME=1
        fi
    else
        FLAG_SET_UNAME=1
    fi
}


# Copy Kernel Image
copy_zImage(){
    echo "Copy Kernel Image"

    if [[ -d /${SD_CARD_ROOT_FS}/boot/ ]]; then
        sudo cp -v ${BBB_U_DIR}/bb-kernel/deploy/${KERNEL_VERSION}.zImage /${SD_CARD_ROOT_FS}/boot/vmlinuz-${KERNEL_VERSION}
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_COPY_ZIMAGE=1
        fi    
    else
        FLAG_COPY_ZIMAGE=1
    fi
}


# Copy Kernel Device Tree Binaries
copy_dtb(){
    echo "Copy Kernel Device Tree Binaries"

    if [[ -d /${SD_CARD_ROOT_FS}/boot ]]; then
        sudo mkdir -p /${SD_CARD_ROOT_FS}/boot/dtbs/${KERNEL_VERSION}/
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_COPY_DTB=1
        fi 
    else
        FLAG_COPY_DTB=1
    fi
    
    sudo tar xfv ${BBB_U_DIR}/bb-kernel/deploy/${KERNEL_VERSION}-dtbs.tar.gz -C /${SD_CARD_ROOT_FS}/boot/dtbs/${KERNEL_VERSION}/
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        FLAG_COPY_DTB=1
    fi
}


# Copy Kernel Modules
copy_modules(){
    echo "Copy Kernel Modules"

    if [[ -d /${SD_CARD_ROOT_FS} ]]; then
        sudo tar xfv ${BBB_U_DIR}/bb-kernel/deploy/${KERNEL_VERSION}-modules.tar.gz -C /${SD_CARD_ROOT_FS}/
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_COPY_MODULES=1
        fi 
    else
        FLAG_COPY_MODULES=1
    fi
}


# File Systems Table (/etc/fstab)
set_file_sys_tables(){
    echo "Set File Systems Table (/etc/fstab)"

    if [[ -d /${SD_CARD_ROOT_FS}/etc ]]; then
        sudo sh -c "echo '${DISK}  /  auto  errors=remount-ro  0  1' >> /${SD_CARD_ROOT_FS}/etc/fstab"
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_SET_FSTAB=1
        fi 
    else
       FLAG_SET_FSTAB=1
    fi
}


# copy MLO
copy_MLO(){
    echo "Copy MLO"

    if [[ -d /${SD_CARD_BOOT} ]]; then
        sudo cp -v ${BBB_U_DIR}/u-boot/MLO /${SD_CARD_BOOT}
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_COPY_MLO=1
        fi
    else
        FLAG_COPY_MLO=1
    fi
}


# copy u-boot.img
copy_uboot_img(){
    echo "Copy u-boot.img"

    if [[ -d /${SD_CARD_BOOT} ]]; then
        sudo cp -v ${BBB_U_DIR}/u-boot/u-boot.img /${SD_CARD_BOOT}
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
            FLAG_COPY_UBOOT=1
        fi
    else
        FLAG_COPY_UBOOT=1
    fi
}


# user menu
PS3="Choose copy operation: "
OPTIONS=( 'Do all'
          'Copy Root File System'
          'Set uname_r in /boot/uEnv.txt'
          'Copy Kernel Image'
          'Copy dtbs'
          'Copy Kernel Modules'
          'Set File Systems Table (/etc/fstab)'
          'Copy MLO'
          'Copy u-boot.img'
          'Exit'
         )

select opt in "${OPTIONS[@]}"
do
    case $opt in
        "Do all")
            copy_root_fs
            set_uname_in_uenv
            copy_zImage
            copy_dtb
            copy_modules
            set_file_sys_tables
            copy_MLO
            copy_uboot_img
            break
            ;;
        "Copy Root File System")
            copy_root_fs
            echo "DONT FORGET TO RUN SYNC!!!!!!!!!!!!!"
            break
            ;;
        "Set uname_r in /boot/uEnv.txt")
            set_uname_in_uenv
            break
            ;;
        "Copy Kernel Image")
            copy_zImage
            break
            ;;
        "Copy dtbs")
            copy_dtb
            break
            ;;
        "Copy Kernel Modules")
            copy_modules
            break
            ;;
        "Set File Systems Table (/etc/fstab)")
            set_file_sys_tables
            break
            ;;
        "Copy MLO")
            copy_MLO
            break
            ;;
        "Copy u-boot.img")
            copy_uboot_img
            break
            ;;
        "Exit")
            break
            ;;
        *)
            echo "invalid option $REPLY"
            break
            ;;
    esac
done


if [ $FLAG_COPY_ROOT_FS -ne 0 ]; then
    echo "***** COPY ROOT FS FAILED!! ******************************************"
fi

if [ $FLAG_SET_UNAME -ne 0 ]; then
    echo "***** SET UNAME IN UENV.TXT FAILED!! *********************************"
fi

if [ $FLAG_COPY_ZIMAGE -ne 0 ]; then
    echo "***** COPY zIMAGE FAILED!! *******************************************"
fi

if [ $FLAG_COPY_DTB -ne 0 ]; then
    echo "***** COPY DTB FAILED!! **********************************************"
fi

if [ $FLAG_COPY_MODULES -ne 0 ]; then
    echo "***** COPY MODULES FAILED!! ******************************************"
fi

if [ $FLAG_SET_FSTAB -ne 0 ]; then
    echo "***** SET FSTAB FAILED!! *********************************************"
fi

if [ $FLAG_COPY_MLO -ne 0 ]; then
    echo "***** COPY MLO FAILED!! **********************************************"
fi

if [ $FLAG_COPY_UBOOT -ne 0 ]; then
    echo "***** COPY UBOOT FAILED!! ********************************************"
fi

echo "Complete!"































