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
 * KVMWithLibvirt.h
 *
 *  Created on: 2013-08-12
 *      Author: Arup Raton Roy (ar3roy@uwaterloo.ca)
 */

#ifndef LIBVIRTATTACHMENT_H_
#define LIBVIRTATTACHMENT_H_

#include "AbstractVM.h"

class KVMWithLibvirt: public AbstractVM {

    string virt_type;
    string networkFile;

    void loadConfiguration(string fileName);
    string createNewImage(unsigned long host_id);
public:
    KVMWithLibvirt(Configurations* globalConf, VMs* vms, CommandExecutor* commandExec, LogicalTopology *topology, Mapping* mapping);
    void createHost2SwitchAttachmentConf();
    void startHost(unsigned long host_id);
    void retrieveInterface(unsigned long host_id);
    void prepare();
    string tapInterfacePrefix();

    virtual ~KVMWithLibvirt();
};

#endif /* LIBVIRTATTACHMENT_H_ */