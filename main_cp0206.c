#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<poll.h>
#include	<errno.h>
#include	<signal.h>
#include	<stdarg.h>
#include	<sys/socket.h>
#include	<sys/types.h>
#include	<bits/types.h>
#include	<arpa/inet.h>
#include	<netinet/if_ether.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
#include	<pthread.h>
#include	<jansson.h>
#include	"netutil.h"
#include	"base.h"
#include	"ip2mac.h"
#include	"sendBuf.h"
#include	"json_config.h"
#include	"tree.h"





PARAM	Param={"net0","net1",0,"10.255.1.1"};
PARAM_new	Param_test1;

struct in_addr	NextRouter;

DEVICE	Device[MAX_DEV_NUM];

int	EndFlag=0;

//struct node *root;

int DebugPrintf(char *fmt,...)
{
	if(Param.DebugOut){
		va_list	args;

		va_start(args,fmt);
		vfprintf(stderr,fmt,args);
		va_end(args);
	}

	return(0);
}

int DebugPerror(char *msg)
{
	if(Param.DebugOut){
		fprintf(stderr,"%s : %s\n",msg,strerror(errno));
	}

	return(0);
}

int SendIcmpTimeExceeded(int deviceNo,struct ether_header *eh,struct iphdr *iphdr,u_char *data,int size){
	struct ether_header	reh;
	struct iphdr	rih;
	struct icmp	icmp;
	u_char	*ipptr;
	u_char	*ptr,buf[1500];
	int	len;

	memcpy(reh.ether_dhost,eh->ether_shost,6);
	memcpy(reh.ether_shost,Device[deviceNo].hwaddr,6);
	reh.ether_type=htons(ETHERTYPE_IP);

	rih.version=4;
	rih.ihl=20/4;
	rih.tos=0;
	rih.tot_len=htons(sizeof(struct icmp)+64);
	rih.id=0;
	rih.frag_off=0;
	rih.ttl=64;
	rih.protocol=IPPROTO_ICMP;
	rih.check=0;
	rih.saddr=Device[deviceNo].addr.s_addr;
	rih.daddr=iphdr->saddr;

	rih.check=checksum((u_char *)&rih,sizeof(struct iphdr));

	icmp.icmp_type=ICMP_TIME_EXCEEDED;
	icmp.icmp_code=ICMP_TIMXCEED_INTRANS;
	icmp.icmp_cksum=0;
	icmp.icmp_void=0;

	ipptr=data+sizeof(struct ether_header);

	icmp.icmp_cksum=checksum2((u_char *)&icmp,8,ipptr,64);

	ptr=buf;
	memcpy(ptr,&reh,sizeof(struct ether_header));
	ptr+=sizeof(struct ether_header);
	memcpy(ptr,&rih,sizeof(struct iphdr));
	ptr+=sizeof(struct iphdr);
	memcpy(ptr,&icmp,8);
	ptr+=8;
	memcpy(ptr,ipptr,64);
	ptr+=64;
	len=ptr-buf;

	DebugPrintf("write:SendIcmpTimeExceeded:[%d] %dbytes\n",deviceNo,len);
	write(Device[deviceNo].soc,buf,len);

	return(0);
}

int AnalyzePacket(int deviceNo,u_char *data,int size,struct node *table_root)
{
	u_char	*ptr;
	int	lest;
	struct ether_header	*eh;
	char	buf[80];
	int	tno;
	u_char	hwaddr[6];

	ptr=data;
	lest=size;

	if(lest<sizeof(struct ether_header)){
		DebugPrintf("[%d]:lest(%d)<sizeof(struct ether_header)\n",deviceNo,lest);
		return(-1);
	}
	eh=(struct ether_header *)ptr;
	ptr+=sizeof(struct ether_header);
	lest-=sizeof(struct ether_header);

	if(memcmp(&eh->ether_dhost,Device[deviceNo].hwaddr,6)!=0){
		DebugPrintf("[%d]:dhost not match %s\n",deviceNo,my_ether_ntoa_r((u_char *)&eh->ether_dhost,buf,sizeof(buf)));
		return(-1);
	}

	if(ntohs(eh->ether_type)==ETHERTYPE_ARP){
		struct ether_arp	*arp;

		if(lest<sizeof(struct ether_arp)){
			DebugPrintf("[%d]:lest(%d)<sizeof(struct ether_arp)\n",deviceNo,lest);
			return(-1);
		}
		arp=(struct ether_arp *)ptr;
		ptr+=sizeof(struct ether_arp);
		lest-=sizeof(struct ether_arp);

		if(arp->arp_op==htons(ARPOP_REQUEST)){
			DebugPrintf("[%d]recv:ARP REQUEST:%dbytes\n",deviceNo,size);
			Ip2Mac(deviceNo,*(in_addr_t *)arp->arp_spa,arp->arp_sha);
		}
		if(arp->arp_op==htons(ARPOP_REPLY)){
			DebugPrintf("[%d]recv:ARP REPLY:%dbytes\n",deviceNo,size);
			Ip2Mac(deviceNo,*(in_addr_t *)arp->arp_spa,arp->arp_sha);
		}
	}
	else if(ntohs(eh->ether_type)==ETHERTYPE_IP){
		struct iphdr	*iphdr;
		u_char	option[1500];
		int	optionLen;

		if(lest<sizeof(struct iphdr)){
			DebugPrintf("[%d]:lest(%d)<sizeof(struct iphdr)\n",deviceNo,lest);
			return(-1);
		}
		iphdr=(struct iphdr *)ptr;
		ptr+=sizeof(struct iphdr);
		lest-=sizeof(struct iphdr);

		optionLen=iphdr->ihl*4-sizeof(struct iphdr);
		if(optionLen>0){
			if(optionLen>=1500){
				DebugPrintf("[%d]:IP optionLen(%d):too big\n",deviceNo,optionLen);
				return(-1);
			}
			memcpy(option,ptr,optionLen);
			ptr+=optionLen;
			lest-=optionLen;
		}

		if(checkIPchecksum(iphdr,option,optionLen)==0){
			DebugPrintf("[%d]:bad ip checksum\n",deviceNo);
			fprintf(stderr,"IP checksum error\n");
			return(-1);
		}

		if(iphdr->ttl-1==0){
			DebugPrintf("[%d]:iphdr->ttl==0 error\n",deviceNo);
			SendIcmpTimeExceeded(deviceNo,eh,iphdr,data,size);
			return(-1);
		}

		tno=(!deviceNo);

		if((iphdr->daddr&Device[tno].netmask.s_addr)==Device[tno].subnet.s_addr){//ifの条件変える
			IP2MAC	*ip2mac;

			DebugPrintf("[%d]:%s to TargetSegment\n",deviceNo,in_addr_t2str(iphdr->daddr,buf,sizeof(buf)));

			if(iphdr->daddr==Device[tno].addr.s_addr){
				DebugPrintf("[%d]:recv:myaddr\n",deviceNo);
				return(1);
			}

			ip2mac=Ip2Mac(tno,iphdr->daddr,NULL);
			if(ip2mac->flag==FLAG_NG||ip2mac->sd.dno!=0){
				DebugPrintf("[%d]:Ip2Mac:error or sending\n",deviceNo);
				AppendSendData(ip2mac,1,iphdr->daddr,data,size);
				return(-1);
			}
			else{
				memcpy(hwaddr,ip2mac->hwaddr,6);
			}
		}
		else{
			IP2MAC	*ip2mac;

			DebugPrintf("[%d]:%s to NextRouter\n",deviceNo,in_addr_t2str(iphdr->daddr,buf,sizeof(buf)));

			
			/*u_int32_t nh_addr;
			struct node *nh;
			nh=longest_match_by_daddr(iphdr->daddr,root);*/
			

			ip2mac=Ip2Mac(tno,NextRouter.s_addr,NULL);
			if(ip2mac->flag==FLAG_NG||ip2mac->sd.dno!=0){
				DebugPrintf("[%d]:Ip2Mac:error or sending\n",deviceNo);
				AppendSendData(ip2mac,1,NextRouter.s_addr,data,size);
				return(-1);
			}
			else{
				memcpy(hwaddr,ip2mac->hwaddr,6);
			}
		}//ここのelseの中を書き換える
		memcpy(eh->ether_dhost,hwaddr,6);
		memcpy(eh->ether_shost,Device[tno].hwaddr,6);

		iphdr->ttl--;
		iphdr->check=0;
		iphdr->check=checksum2((u_char *)iphdr,sizeof(struct iphdr),option,optionLen);

		write(Device[tno].soc,data,size);
	}

	return(0);
}

int Router(struct node *table_root)
{
	struct pollfd	targets[2];
	int	nready,i,size;
	u_char	buf[2048];

	targets[0].fd=Device[0].soc;
	targets[0].events=POLLIN|POLLERR;
	targets[1].fd=Device[1].soc;
	targets[1].events=POLLIN|POLLERR;

	while(EndFlag==0){
		switch(nready=poll(targets,2,100)){
			case	-1:
				if(errno!=EINTR){
					DebugPerror("poll");
				}
				break;
			case	0:
				break;
			default:
				for(i=0;i<2;i++){
					if(targets[i].revents&(POLLIN|POLLERR)){
						if((size=read(Device[i].soc,buf,sizeof(buf)))<=0){
							DebugPerror("read");
						}
						else{
							AnalyzePacket(i,buf,size,table_root);
						}
					}
				}
				break;
		}
	}

	return(0);
}

int DisableIpForward()
{
	FILE    *fp;

	if((fp=fopen("/proc/sys/net/ipv4/ip_forward","w"))==NULL){
		DebugPrintf("cannot write /proc/sys/net/ipv4/ip_forward\n");
		return(-1);
	}
	fputs("0",fp);
	fclose(fp);

	return(0);
}

void *BufThread(void *arg)
{
	BufferSend();
	return(NULL);
}

void EndSignal(int sig)
{
	EndFlag=1;
}

pthread_t	BufTid;

int main(int argc,char *argv[],char *envp[])
{
	char	buf[80];
	pthread_attr_t	attr;
	int	status,i;

	json_t json_object;
	json_error_t jerror;
	struct node *root;
	root=malloc(sizeof(struct node));
	init_tree_node(root);
	root->is_empty=1;
	root->is_root=1;

	init_PARAM_new(&Param_test1);

	json_read(&Param_test1,&json_object,&jerror,root);

	show_tree(root);


	struct node *nh;
	struct in_addr nh_addr;
	nh=longest_match_by_daddr(inet_addr("192.168.12.2"),root);
	
	if(nh==NULL){
		printf("null\n");
	}
	else{
		nh_addr.s_addr=nh->next_hop;
		printf("example nexthop:%s\n",inet_ntoa(nh_addr));
	}


	printf("%s\n",Param_test1.Device[0]);

	inet_aton(Param.NextRouter,&NextRouter);
	DebugPrintf("NextRouter=%s\n",my_inet_ntoa_r(&NextRouter,buf,sizeof(buf)));
	
	for(i=0;i<(Param_test1.num_of_dev);i++){
		if(GetDeviceInfo(Param_test1.Device[i],Device[i].hwaddr,&Device[i].addr,&Device[i].subnet,&Device[i].netmask)==-1){
			DebugPrintf("GetDeviceInfo:error:%s\n",Param.Device1);
			printf("free of tree\n");
			tree_destruct(root);
			return(-1);
		}
		if((Device[0].soc=InitRawSocket(Param_test1.Device[i],0,0))==-1){
			DebugPrintf("InitRawSocket:error:%s\n",Param_test1.Device[i]);
			printf("free of tree\n");
			tree_destruct(root);
			return(-1);
		}
		DebugPrintf("%s OK\n",Param_test1.Device[i]);
		DebugPrintf("addr=%s\n",my_inet_ntoa_r(&Device[0].addr,buf,sizeof(buf)));
		DebugPrintf("subnet=%s\n",my_inet_ntoa_r(&Device[0].subnet,buf,sizeof(buf)));
		DebugPrintf("netmask=%s\n",my_inet_ntoa_r(&Device[0].netmask,buf,sizeof(buf)));
	}
	

	DisableIpForward();

	pthread_attr_init(&attr);
	if((status=pthread_create(&BufTid,&attr,BufThread,NULL))!=0){
		DebugPrintf("pthread_create:%s\n",strerror(status));
	}

	signal(SIGINT,EndSignal);
	signal(SIGTERM,EndSignal);
	signal(SIGQUIT,EndSignal);

	signal(SIGPIPE,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);

	DebugPrintf("router start\n");
	Router(root);
	DebugPrintf("router end\n");

	pthread_join(BufTid,NULL);

	close(Device[0].soc);
	close(Device[1].soc);

	//free() of Device

	printf("free of tree\n");
	tree_destruct(root);

	return(0);
}