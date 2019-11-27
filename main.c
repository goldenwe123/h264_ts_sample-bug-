#include <stdio.h>
#include <stdlib.h>
#define BUFFER_SIZE 188


typedef struct {
	
	unsigned char sync_byte;
	unsigned char transport_error_indicator;
	unsigned char payload_unit_start_indicator;
	unsigned char transport_priority;
	unsigned short pid;
	unsigned char transport_scrambling_control;
	unsigned char adaptation_field_control;
	unsigned char continuity_counter ;

}Ts_header;

typedef struct {
	
	unsigned char flag;
	unsigned char stream_id;
	unsigned short packet_length;
	unsigned char flag_priority;
	unsigned char flag_pts;
	unsigned char flag_dts;
	unsigned char data_length;
	unsigned char pts[5];
	unsigned char dts[5];

}Pes_header;

int is_continuous_zero(unsigned char *buf,int p){
	
	if(buf[p] == 0x00 && buf[p+1] == 0x00 ){
		
		
		if(buf[p+2] == 0x01 )
			return p+3;
		else if(buf[p+2] == 0x00 && buf[p+3] == 0x01)
			return p+4;
	
	}
	
	return 0;
}

int is_frame(unsigned char *buf,int p){
	
	int ret;
	int val;
	int is_find_i=0;
	ret=is_continuous_zero(buf,p);
	
	
	if(ret) {
		
		val=buf[ret] & 0x1f;
		
		
		
		if(!is_find_i){
			if(val == 7 || val == 5){
				is_find_i=1;
				return ret;
			}	
		}

		if(val == 1){
				is_find_i=0;
				return ret;
		}	
		
	}	
	
	
	return 0;
}

int write_ts_header(Ts_header *p,FILE *fp_w){
	
	char temp;
	static char count=0;
	count=(count + 1) & 0x0f;
	p->continuity_counter = count;
	fwrite(&p->sync_byte, 1, 1, fp_w);
	temp = ( p->transport_error_indicator << 7 ) + ( p->payload_unit_start_indicator << 6 )
				+ ( p->transport_priority << 5 ) + ( p->pid >> 8 );
	fwrite(&temp, 1, 1, fp_w);
	temp = p->pid & 0xfff;
	fwrite(&temp, 1, 1, fp_w);
	temp = ( p->transport_scrambling_control << 6 ) + ( p->adaptation_field_control << 4 )+ p->continuity_counter;
	//printf("%x\n", temp  );
	fwrite(&temp, 1, 1, fp_w);


	return 4;
}

int write_pes_header(Pes_header *pes,FILE *fp_w){
	
	int frame_rate=8;
	int pts_offset=90000/frame_rate;
	static unsigned long pts=90000;
	char temp;
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x01;
	fwrite(&temp, 1, 1, fp_w);	
	temp=pes->stream_id;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=pes->flag_priority;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x80;
	fwrite(&temp, 1, 1, fp_w);
	temp=pes->data_length;
	fwrite(&temp, 1, 1, fp_w);
	
	pts = pts + pts_offset;
	pes->pts[0] = (pts & 0xff00000000) >> 32;
	pes->pts[1] = (pts & 0x00ff000000) >> 24;
	pes->pts[2] = (pts & 0x0000ff0000) >> 16;
	pes->pts[3] = (pts & 0x000000ff00) >> 8;
	pes->pts[4] = (pts & 0x00000000ff);

	temp=pes->pts[0];
	fwrite(&temp, 1, 1, fp_w);
	temp=pes->pts[1];
	fwrite(&temp, 1, 1, fp_w);
	temp=pes->pts[2];
	fwrite(&temp, 1, 1, fp_w);
	temp=pes->pts[3];
	fwrite(&temp, 1, 1, fp_w);
	temp=pes->pts[4];
	fwrite(&temp, 1, 1, fp_w);

	return 14;
}

int write_splite(FILE *fp_w) {
	
	char temp;
	
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x00;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x01;
	fwrite(&temp, 1, 1, fp_w);
	temp=0x09;
	fwrite(&temp, 1, 1, fp_w);
	temp=0xf0;
	fwrite(&temp, 1, 1, fp_w);
	
	

	return 6;
}	


int main(){
	
	Ts_header *p=malloc(sizeof(Ts_header));
	p->sync_byte = 0x47;
	p->transport_error_indicator = 0;
	p->payload_unit_start_indicator = 0;
	p->transport_priority = 0;
	p->pid = 4001;
	p->transport_scrambling_control = 0;
	p->adaptation_field_control = 1;
	p->continuity_counter = 0;
	
	Pes_header *pes=malloc(sizeof(Pes_header));
	pes->flag=1;
	pes->stream_id=0xe0;
	pes->packet_length=0;
	pes->flag_priority=0x80;
	pes->flag_pts=1;
	pes->flag_dts=0;
	pes->data_length=5;

	
	FILE *fp_r;
	FILE *fp_w;
	char *buf;
	int file_size;
	fp_r = fopen( "test.h264","rb" );
	
	fseek(fp_r, 0L, SEEK_END);
	file_size = ftell(fp_r);
	rewind(fp_r);
	printf(" file size: %d\n",file_size );	
	buf = (char *)malloc(file_size); 
	int size = fread(buf, 1, file_size, fp_r);
	printf(" read size: %d\n",size  );
	
	
	fp_w=fopen("out.ts","wb");
	
	
	unsigned int i=0;
	int n=0;

	while(i<size){
		
		int write_count=0;
		int ret=is_frame(buf,i);
		int isFrame=0;
		
		
		if(ret) {
		
			p->payload_unit_start_indicator = 1;
			write_count = write_count + 4;
			write_count = write_count + 14;
			write_count = write_count + 6;
			isFrame=1;
		
					
		}else{
			ret=i;
			p->payload_unit_start_indicator = 0;
			write_count = write_count + 4;
		}	
		//printf(" write_count: %d\n",write_count  );
		int write_nums=ret-i;
		//printf("ret %d i %d write_nums: %d\n",ret,i,write_nums  );
		//printf("****\n");
		
		for(int j=ret;j<i+(188-write_count);j++){

			if(is_frame(buf,j))
					break;
			write_nums++;

		}
		//printf("write_nums: %d\n",write_nums  );
		int stuff_nums=188 - (write_nums + write_count);
		int temp;
		if(stuff_nums){
			
			p->adaptation_field_control = 3;
		
			
		}else{
			p->adaptation_field_control = 1;
		}
		
		write_ts_header(p,fp_w);
		
		if(stuff_nums){
			
			temp=stuff_nums-1;
			fwrite(&temp, 1, 1, fp_w);
			
			if(stuff_nums!=1){
				temp=0;
				fwrite(&temp, 1, 1, fp_w);
			}
		}	
		
		
		if(isFrame){
			write_pes_header(pes,fp_w);
			write_splite(fp_w);
			
		}	
		
		temp=0xff;
		for(int j=0;j<(stuff_nums-2);j++)
			fwrite(&temp, 1, 1, fp_w);
		
		
		fwrite(&buf[i], 1, write_nums, fp_w);
		
		//printf(" stuff_nums: %d\n",stuff_nums  );
		
		if((i+write_nums)<=i){
			printf("error\n");
			break;
			
		}	
						
			//printf("i %d n %d write_nums: %d\n",i,n,write_nums  );
		i=i+write_nums;
	
		
				
	}	
	printf(" Done\n");

}	
