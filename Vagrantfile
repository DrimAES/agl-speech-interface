# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/xenial64"

  config.vm.provider "virtualbox" do |vb|
    vb.memory = "2048"
  end

  config.vm.network "forwarded_port", guest: 1235, host: 1235

  config.vm.provision "shell", inline: <<-SHELL

    export DISTRO="xUbuntu_16.04"
    wget -O - http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Development/${DISTRO}/Release.key | sudo apt-key add -
    
    echo "deb http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Development/${DISTRO}/ ./" >> /etc/apt/sources.list.d/AGL.list
    echo "deb http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Framework/${DISTRO}/ ./" >> /etc/apt/sources.list.d/AGL.list

    apt-get update
    apt-get install -y \
      build-essential \
      git \
      curl \
      wget \
      cmake \
      pkg-config \
      libjson-c-dev \
      libsystemd-dev \
      agl-xds-agent \
      agl-xds-exec \
      agl-app-framework-binder-bin \
      agl-app-framework-binder-dev

    apt-get -y dist-upgrade

    # Source the profile script in bashrc so that it is also available
    # for non-login shells
    echo ". /etc/profile.d/AGL_app-framework-binder.sh" >> /etc/bash.bashrc

    mkdir -p $HOME/tmp
    cd $HOME/tmp
    wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.55.tar.gz
    tar xzf libmicrohttpd-0.9.55.tar.gz
    cd libmicrohttpd-0.9.55
    ./configure && make && sudo make install
    cd ..

  SHELL
end
