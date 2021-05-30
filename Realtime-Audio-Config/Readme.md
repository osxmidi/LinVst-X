Some distros such as Manjaro/Mint don't seem to currently setup audio for realtime 

If they are not set then cpu spiking can occur with plugins.

To set audio realtime priorities edit the below files.

```

sudo edit /etc/security/limits.conf

add

@audio - rtprio 99

```

```

run from Terminal

sudo usermod -a -G audio your-user-name

```

```

sudo edit /etc/security/limits.d/audio.conf

add

@audio - rtprio 95
@audio - memlock unlimited
#@audio - nice -19


Also, installing the rtirq-init (rtirq for Manjaro) and irqbalance packages might be useful.

rtirq is enabled when threadirqs is added to grub

sudo edit /etc/default/grub 
GRUB_CMDLINE_LINUX="threadirqs"
sudo update-grub

adding "xhci_hcd" to RTIRQ_NAME_LIST in the rtirq.conf file might be needed
sudo edit /etc/rtirq.conf 
#RTIRQ_NAME_LIST="snd usb i8042"
RTIRQ_NAME_LIST="xhci_hcd"

After reboot, check rtirq status
sudo /usr/bin/rtirq status
or sudo /etc/init.d/rtirq status
