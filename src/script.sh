#!/bin/sh

MODULE=cfan
RULE="99-$MODULE.rules"
ID_VENDOR="0c45"
ID_PRODUCT="7701"

# Creation of the udev rule file
if [ ! -e "$RULE" ] ; then
	echo -n "ATTRS{idVendor}==\"$ID_VENDOR\", " >> $RULE;
	echo "ATTRS{idProduct}==\"$ID_PRODUCT\", \\" >> $RULE;
	echo -n "PROGRAM=\"/bin/sh -c " >> $RULE;
	echo "'echo -n \$id:1.0 >/sys/bus/usb/drivers/usbhid/unbind;\\" >> $RULE;
	echo "echo -n \$id:1.0 > /sys/bus/usb/drivers/$MODULE/bind'\"" >> $RULE
fi

# Build the module
make clean && make && \
	echo "\n--> Make '$MODULE' has been done."

# Reset the module
if [ ! -z "$(lsmod | grep $MODULE)" ] ; then
	sudo rmmod $MODULE && \
		echo "--> Module '$MODULE' has been removed."
else
	sudo insmod $MODULE\.ko && \
		echo "--> Module '$MODULE' has been added."
fi

# Add the rule if it is not exist
if [ -z `ls /etc/udev/rules.d/ | grep $RULE` ] ; then
	sudo cp 99-cfan.rules /etc/udev/rules.d/ && \
		sudo udevadm control --reload-rules && \
		echo "--> Rule '$RULE' has been added."
else
	echo "--> Rule '$RULE' has already been added."
fi

# Tests
cd ../tests/
make -f Makefile.test clean && make -f Makefile.test
if [ $? -eq 0 ]; then
	echo "--> Make 'test' has been done.";
else
	echo "xxx Make 'test' has failed !";
	exit 1
fi

sudo ./test

err=$?

if [ $err -eq 2 ]; then
	echo "xxx Error[$err]: Test has failed (unable to open /dev/cfan0) !"
	if [ ! -f /dev/cfan0 ]; then
		echo "xxx Error: '/dev/cfan0' does not exist !"
	fi
elif [ $err -eq 3 ]; then
	echo "xxx Error[$err]: Test has failed (unable to write on /dev/cfan0) !"
elif [ $err != 0 ]; then
	echo "xxx Error[$err]: Test has failed !"
else
	echo "--> Test has succeeded !"
fi

