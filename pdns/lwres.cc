/*
    PowerDNS Versatile Database Driven Nameserver
    Copyright (C) 2002 - 2005 PowerDNS.COM BV

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as 
    published by the Free Software Foundation

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "utility.hh"
#include "lwres.hh"
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <errno.h>
#include "misc.hh"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <string>
#include <vector>
#include "dnspacket.hh"
#include "dns.hh"
#include "qtype.hh"
#include "tcpreceiver.hh"
#include "ahuexception.hh"
#include "statbag.hh"
#include "arguments.hh"
#include "sstuff.hh"
#include "syncres.hh"

LWRes::LWRes()
{
  d_sock=-1;
  d_timeout=500000;
  d_bufsize=1500;
  d_buf=new unsigned char[d_bufsize];
}

LWRes::~LWRes()
{
  if(d_sock>=0)
    Utility::closesocket(d_sock);
  delete[] d_buf;
}


//! returns -1 for permanent error, 0 for timeout, 1 for success
/** Never throws! */
int LWRes::asyncresolve(const string &ip, const char *domain, int type, bool doTCP)
{
  DNSPacket p;
  p.setQuestion(Opcode::Query,domain,type);
  p.setRD(false);
  p.wrapup();

  d_domain=domain;
  d_type=type;
  d_inaxfr=false;
  d_rcode=0;

  struct sockaddr_in toaddr;
  struct in_addr inp;
  Utility::socklen_t addrlen=sizeof(toaddr);
  Utility::inet_aton(ip.c_str(),&inp);
  toaddr.sin_addr.s_addr=inp.s_addr;

  toaddr.sin_port=htons(53);
  toaddr.sin_family=AF_INET;

  int ret;

  DTime dt;
  dt.set();

  if(!doTCP) {
    if(asendto(p.getData(), p.len, 0, (struct sockaddr*)(&toaddr), sizeof(toaddr),p.d.id)<0) {
      return -1;
    }
  
    // sleep until we see an answer to this, interface to mtasker
    
    ret=arecvfrom(reinterpret_cast<char *>(d_buf), d_bufsize-1,0,(struct sockaddr*)(&toaddr), &addrlen, &d_len, p.d.id);
  }
  else {
    //    cerr<<"do tcp"<<endl;
    Socket s(InterNetwork, Stream);
    IPEndpoint ie(ip, 53);
    s.setNonBlocking();
    s.connect(ie);

    unsigned int len=htons(p.len);
    char *lenP=(char*)&len;
    const char *msgP=p.getData();
    string packet=string(lenP, lenP+2)+string(msgP, msgP+p.len);

    if(asendtcp(packet, &s) == 0) {
      //      cerr<<"asendtcp: timeout"<<endl;
      return 0;
    }
    
    packet.clear();
    if(arecvtcp(packet,2, &s)==0) {
      //      cerr<<"arecvtcp: timeout"<<endl;
      return 0;
    }

    memcpy(&len, packet.c_str(), 2);
    len=ntohs(len);

    //    cerr<<"Now reading "<<len<<" bytes"<<endl;

    if(arecvtcp(packet, len, &s)==0) {
      //      cerr<<"arecvtcp: timeout"<<endl;
      return 0;
    }
    if(len > d_bufsize) {
      d_bufsize=len;
      delete[] d_buf;
      d_buf = new unsigned char[d_bufsize];
    }
    memcpy(d_buf, packet.c_str(), len);
    d_len=len;
    ret=1;
  }
  d_usec=dt.udiff();
    
  return ret;
}


LWRes::res_t LWRes::result()
{
  DNSPacket p;

  try {
    if(p.parse((char *)d_buf, d_len)<0)
      throw LWResException("resolver: unable to parse packet of "+itoa(d_len)+" bytes");
    d_aabit=p.d.aa;
    d_tcbit=p.d.tc;
    d_rcode=p.d.rcode;
    return p.getAnswers();
  }
  catch(...) {
    d_rcode=RCode::ServFail;
    LWRes::res_t empty;
    return empty;
  }
}

