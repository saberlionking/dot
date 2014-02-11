/*
* This file is part of DOT.
*
*     DOT is free software: you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation, either version 3 of the License, or
*     (at your option) any later version.
*
*     DOT is distributed in the hope that it will be useful,
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*     GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with DOT.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright 2011-2013 dothub.org
*/

/*
 * LibvirtAttachment.cpp
 *
 *  Created on: 2013-08-12
 *      Author: Arup Raton Roy (ar3roy@uwaterloo.ca)
 */

#include "LibvirtAttachment.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "Util.h"

using namespace std;

LibvirtAttachment::LibvirtAttachment(Configurations* globalConf, Hosts* hosts, CommandExecutor* commandExec, InputTopology* topology, Mapping* mapping)
    :InstantitiateHost(globalConf, hosts, commandExec, topology, mapping)
{
    this->loadConfiguration(globalConf->hypervisorConfigurationFile);

}

LibvirtAttachment::~LibvirtAttachment() {

}
void LibvirtAttachment::prepare()
{
    this->createHost2SwitchAttachmentConf();
}
void LibvirtAttachment::loadConfiguration(string fileName)
{
    map<string, string> eachConfiguration;

    virt_type = "kvm";
    networkFile="libvirt_network.xml";

    ifstream fin(fileName.c_str());

    if(fin.is_open())
    {
        while(!fin.eof())
        {
            string line, key, value;
            fin >> line;
            Util::parseKeyValue(line, key, value, '=');

            if(key.compare(""))
            {
                //cout << key << endl << value << endl;
                eachConfiguration[key] = value;
            }
        }

        fin.close();
    }
    else
        cout << "Unable to open lib-virt configuration file: " << fileName <<  endl;

    if(eachConfiguration.find("virt_type") != eachConfiguration.end())
        virt_type = eachConfiguration["virt_type"];

    if(eachConfiguration.find("networkFile") != eachConfiguration.end())
        networkFile = eachConfiguration["networkFile"];

}
void LibvirtAttachment::createHost2SwitchAttachmentConf()
{

    for(unsigned long i = 0; i < this->topology->getNumberOfSwitches(); i++)
    {
        ifstream fin(this->networkFile.c_str(), ios::binary);
        if(fin.is_open())
        {

            vector<string> * tokens;
            Util::parseString(this->networkFile, tokens, '.');

            ostringstream outputFileName;
            outputFileName << (*tokens)[0] << i+1 << "." << (*tokens)[1];

            ofstream fout(outputFileName.str().c_str(), ios::binary);

            if(fout.is_open())
            {
                fout << fin.rdbuf();
                fout.close();

                cout << i+1 << endl;

                //Replacing the $ with the ovs id
                ostringstream command;
                command << "sed -i \'s/\\$/" << i+1 << "/g\' " << outputFileName.str();
                this->commandExec->executeLocal(command.str());

                this->commandExec->copyContent("localhost", this->mapping->getMachine(i), outputFileName.str(), outputFileName.str());

                ostringstream netDefineCommand;

                netDefineCommand.str("");
                netDefineCommand << "sudo virsh net-destroy ovs_network_" << i+1;
                this->commandExec->executeRemote(this->mapping->getMachine(i), netDefineCommand.str());

                netDefineCommand.str("");
                netDefineCommand << "sudo virsh net-undefine ovs_network_" << i+1;
                this->commandExec->executeRemote(this->mapping->getMachine(i), netDefineCommand.str());

                netDefineCommand.str("");
                netDefineCommand << "sudo virsh net-define " << outputFileName.str();
                this->commandExec->executeRemote(this->mapping->getMachine(i), netDefineCommand.str());

                netDefineCommand.str("");
                netDefineCommand << "sudo virsh net-start ovs_network_" << i+1;
                this->commandExec->executeRemote(this->mapping->getMachine(i), netDefineCommand.str());

                netDefineCommand.str("");
                netDefineCommand << "sudo virsh net-autostart ovs_network_" << i+1;
                this->commandExec->executeRemote(this->mapping->getMachine(i), netDefineCommand.str());

            }
            else
                cout << "Unable to create network configuration file: " << outputFileName.str() <<  endl;

            fin.close();
        }
        else
            cout << "Unable to open network configuration template file: " << this->networkFile <<  endl;
    }
}

string LibvirtAttachment::tapInterfacePrefix()
{
    return "vnet";
}

string LibvirtAttachment::createNewImage(unsigned long host_id)
{

    unsigned long switchId = this->hosts->getSwitch(host_id);
    string imagePath = Util::getPathName(this->globalConf->hostImage);

    ostringstream newImageName;
    newImageName << imagePath << "clone_H" << host_id+1 << ".qcow2";
    ostringstream command;

    command << "sudo qemu-img create -b ";
    command <<  this->globalConf->hostImage;
    command << " -f qcow2 ";
    command << newImageName.str();

    this->commandExec->executeRemote(this->mapping->getMachine(switchId), command.str());


    return newImageName.str();
}
void LibvirtAttachment::startHost(unsigned long host_id)
{
    unsigned long switchId = this->hosts->getSwitch(host_id);
    string mac = this->hosts->getMacAddress(host_id);

    ostringstream command;

    command << "sudo virsh undefine H" << host_id+1;
    this->commandExec->executeRemote(this->mapping->getMachine(switchId), command.str());
    command.str("");
    command << "sudo virsh destroy H" << host_id+1;
    this->commandExec->executeRemote(this->mapping->getMachine(switchId), command.str());
    command.str("");
    command << "sudo virt-install";

    if(this->virt_type.compare("kvm") == 0)
    {
            command << " --connect qemu:///system --virt-type kvm";
    }

    command << " --name H" << host_id+1;
    command << " --ram 64 ";
    command << " --disk  path="<< this->createNewImage(host_id)<<",format=qcow2";
    command << " --vnc --noautoconsole";
    command << " --network network=ovs_network_" << switchId+1 << ",mac="<< mac;
    command << " --boot hd";

    this->commandExec->executeRemote(this->mapping->getMachine(switchId), command.str());

}

