#!/bin/bash

# SD Card format:
# BOOT:     MLO, u-boot.img
# ROOT_FS:  RFS, uEnv.txt, zImage, dtb, modules, file system table


KERNEL_VERSION=5.4.109-bone48
BBB_U_DIR=../

DISK=/dev/sdb
SD_CARD_PATH=media/${USER}
SD_CARD_ROOT_FS=${SD_CARD_PATH}/ROOT_FS
SD_CARD_BOOT=${SD_CARD_PATH}/BOOT


# Copy Root File System
copy_root_fs() {
    echo "Copying root file system"
    sudo tar xfvp ${BBB_U_DIR}/*-*-*-armhf-*/armhf-rootfs-*.tar -C /${SD_CARD_ROOT_FS}/
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
    
    sudo chown root:root /${SD_CARD_ROOT_FS}
    sudo chmod 755 /${SD_CARD_ROOT_FS}
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
}


# Set uname_r in /boot/uEnv.txt
set_uname_in_uenv(){
    echo "Set uname_r in /boot/uEnv.txt"
    sudo sh -c "echo 'uname_r=${KERNEL_VERSION}' >> /${SD_CARD_ROOT_FS}/boot/uEnv.txt"
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
}


# Copy Kernel Image
copy_zImage(){
    echo "Copy Kernel Image"
    sudo cp -v ${BBB_U_DIR}/bb-kernel/deploy/${KERNEL_VERSION}.zImage /${SD_CARD_ROOT_FS}/boot/vmlinuz-${KERNEL_VERSION}
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
}


# Copy Kernel Device Tree Binaries
copy_dtb(){
    echo "Copy Kernel Device Tree Binaries"
    sudo mkdir -p /${SD_CARD_ROOT_FS}/boot/dtbs/${KERNEL_VERSION}/
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
    
    sudo tar xfv ${BBB_U_DIR}/bb-kernel/deploy/${KERNEL_VERSION}-dtbs.tar.gz -C /${SD_CARD_ROOT_FS}/boot/dtbs/${KERNEL_VERSION}/
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
}


# Copy Kernel Modules
copy_modules(){
    echo "Copy Kernel Modules"
    sudo tar xfv ${BBB_U_DIR}/bb-kernel/deploy/${KERNEL_VERSION}-modules.tar.gz -C /${SD_CARD_ROOT_FS}/
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
    fi
}


# File Systems Table (/etc/fstab)
set_file_sys_tables(){
    echo "Set File Systems Table (/etc/fstab)"
    sudo sh -c "echo '${DISK}  /  auto  errors=remount-ro  0  1' >> /${SD_CARD_ROOT_FS}/etc/fstab"
    RESULT=$?
    if [ $RESULT -ne 0 ]; then
        echo "ERROR"
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
        "Exit")
            break
            ;;
        *)
            echo "invalid option $REPLY"
            break
            ;;
    esac
done

echo "Complete!"










