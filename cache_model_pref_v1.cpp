#include<iostream>
#include<math.h>
#include<stdlib.h>
#include<stdio.h>
#include<string>
#include <sstream> // For passing command line parameters using stringstream
#include<fstream> // For file operations
#include<vector> // For file operations




FILE *pFile = fopen ("out.txt","w");
using namespace std;
//namespace fs = filesystem;

typedef struct cache_blk
{
    short int c_valid;
    unsigned long long int c_tag;
    int c_value;
    int lru_value;
} ;




//global vars
int nk;//512 Kbytes
int assoc ;//4 way
int blk_size;//64 bytes;
char repl;


int n_blks;
int n_sets;

int blk_offset;
int set_index;
int tag_index;

unsigned long long int blk_mask;
unsigned long long int set_mask;
unsigned long long int tag_mask; 

unsigned long long int blk;
unsigned long long int set;
unsigned long long int tag; 



unsigned long long int r_addr;
char r_op;

unsigned int r_hit =0;
unsigned int r_miss =0;
unsigned int w_hit =0;
unsigned int w_miss =0;
unsigned int n_pref =0;

//LRU Status
void lru_status(struct cache_blk **mem,unsigned long long int set)
{
//print LRU status
cout<<"\n**LRU Stats**";
for(int i=0;i<=assoc-1;i++)
{

printf("\n lru=%d -- c_valid = %d -- c_value=%d",mem[set][i].lru_value,mem[set][i].c_valid, mem[set][i].c_value); 

}

cout<<endl;
}


//LRU_update on hit
void lru_update_hit(int col_index,struct cache_blk **mem,unsigned long long int set)
{




int curr_lru_value = mem[set][col_index].lru_value;
mem[set][col_index].lru_value = 1;

//printf("\n curr_lru_value = %d",curr_lru_value);
if(curr_lru_value == 0)
{
    for(int i=0;i<=assoc-1;i++)
    {

    if( mem[set][i].c_valid !=0  && i!=col_index )
    {
    //printf("\n valid = %d",mem[set][i].c_valid);
    //printf("\n i=%d -- lru_curr = %d",i,mem[set][i].lru_value);
    mem[set][i].lru_value ++;

    }

    }
}

else 
{
for(int i=0;i<=assoc-1;i++)
{
    if(mem[set][i].lru_value < curr_lru_value && mem[set][i].c_valid ==1 && i!=col_index) 
        {
            mem[set][i].lru_value ++;
        }

}

}



}


//LRU_update on replace
int lru_replace(struct cache_blk **mem,unsigned long long int set)
{

int rep_blk = -1;
//chk if empty available
for(int i=0;i<=assoc-1;i++)
{

if(mem[set][i].c_valid ==0)
{
    return(i);
}
else if(mem[set][i].lru_value == assoc)
	rep_blk = i;
}

return(rep_blk);



}


//RAND replace
int rand_replace(struct cache_blk **mem,unsigned long long int set)
{

//chk if empty available
for(int i=0;i<=assoc-1;i++)
{

if(mem[set][i].c_valid ==0)
{
    return(i);
}

}
//no empty found

return(rand()%assoc);

}

int cache_op(unsigned long long int addr,char op,int rw_value,struct cache_blk **mem)
{
//unsigned long long int addr=0x0FF00000000007D0;

blk = addr & blk_mask;
set = (addr & set_mask) >> blk_offset;
tag = (addr & tag_mask) >> (64-tag_index);


//printf("\naddr = 0x%016llx",addr);
//printf("\nTag = 0x%016llx",tag);
//printf("\nSet= 0x%016llx",set);
//printf("\nOffset = 0x%016llx",blk);
//printf("\nRWValue = %lld",rw_value);


//Cache Hit	
int f_hit = 0;

for(int i=0;i<=assoc-1;i++)
{

if(mem[set][i].c_tag == tag  && mem[set][i].c_valid ==1)
    {
    f_hit =1;
	
	if(op == 'r')
	{
		r_hit++;
//		cout<<endl<<"read hit";
//		fprintf(pFile,"read hit");

	}
	else if(op == 'w')
	{
		w_hit++;
//		cout<<endl<<"write hit";
//		fprintf(pFile,"write hit");
	}
	
    //update LRU
    if(repl == 'L')
    	lru_update_hit(i,mem,set);
   
    //lru_status(mem,set);
    break;
    } 


}

//Cache Miss
if(f_hit == 0)
{
	
	if(op == 'r')
	{
		r_miss ++;
//		cout<<endl<<"read miss";
//		fprintf(pFile,"read miss");
	}
	else if(op == 'w')
	{
		w_miss++;
//		cout<<endl<<"write miss";
//		fprintf(pFile,"write miss");

	}
	
    int blk_replace;
//    lru_status(mem,set);


    //replace LRU logic
    if(repl == 'L')
    {
	
    	blk_replace = lru_replace(mem,set);
    	lru_update_hit(blk_replace,mem,set);
    }
    else
    	blk_replace = rand_replace(mem,set);//replace random logic
    

//    printf("\n blk_replace = %d,c_value=%d\n",blk_replace, mem[set][blk_replace].c_value);
    mem[set][blk_replace].c_tag = tag;
    mem[set][blk_replace].c_value = rw_value;
    mem[set][blk_replace].c_valid =1;

//    lru_status(mem,set);

}



//cout<< endl;
return(1);
} //end cache_op



// Prefetcher
void cache_prefetch(unsigned long long int N,unsigned long long int addr,struct cache_blk **mem)
{
//unsigned long long int addr=0x0FF00000000007D0;

blk = addr & blk_mask;
set = (addr & set_mask) >> blk_offset;
tag = (addr & tag_mask) >> (64-tag_index);


//printf("\n Prefetcher Start \n");

//printf("\naddr = 0x%016llx",addr);
//printf("\nTag = 0x%016llx",tag);
//printf("\nSet= 0x%016llx",set);
//printf("\nOffset = 0x%016llx",blk);
//printf("\nRWValue = %lld",rw_value);

for(int line =1;line<=N;line++)
{

//printf("\n set %llx, max= %llx",set+line,n_sets);

if(set+line <= n_sets-N)
{

//Next line hit
int f_hit = 0;

for(int i=0;i<=assoc-1;i++)
{

if(mem[set + line][i].c_tag == tag  && mem[set + line][i].c_valid ==1)
    {
 	
	//already present
	f_hit=1;
	
    break;
    } 


}

//Next line miss
if(f_hit == 0)
{
	
	n_pref++;
    int blk_replace;
//    lru_status(mem,set+line);


    //replace LRU logic
    if(repl == 'L')
    {
	
    	blk_replace = lru_replace(mem,set+line);
    	lru_update_hit(blk_replace,mem,set+line);
    }
    else
    	blk_replace = rand_replace(mem,set);//replace random logic
    

//    printf("\n Prefetcher blk_replace = %d,c_value=%d\n",blk_replace, mem[set+line][blk_replace].c_value);
    mem[set + line][blk_replace].c_tag = tag;
    mem[set + line][blk_replace].c_value = rand();
    mem[set + line][blk_replace].c_valid =1;

//    lru_status(mem,set+line);
    
//    printf("\n Prefetcher End \n");

}


}

//cout<< endl;

} //end cache_prefetch

}


int main()//int argc,char *argv[]
{

nk = 2048; //512 Kbytes
assoc = 64; //4 way
blk_size = 64; //64 bytes;
repl = 'L';
int Np = 1;

//nk = 512; //512 Kbytes
//assoc = 4; //4 way
//blk_size = 64; //64 bytes;
//repl = 'R';
//int Np = 2;

///* Cache size */
//nk = atoi (argv[1]);
///* Associvity  */
//assoc = atoi (argv[2]);
///* blocksize */
//blk_size = atoi (argv[3]);
///* replacement policy LRU or Random */
//repl = argv[4][0];


n_blks = (nk*1024)/blk_size;
n_sets = n_blks/assoc;
blk_offset = log2(blk_size);
set_index = log2(n_sets);
tag_index = 64-set_index-blk_offset;

printf("cache_size = %d Kilo bytes\n",nk);
printf("assoc = %d ways\n",assoc);
printf("blk_size = %d bytes\n",blk_size);
printf("repl = %c\n",repl);
printf("n_blks = %d, n_sets = %d \n",n_blks,n_sets);
printf("tag_bits = %d, set_index = %d ,blk_offset = %d\n",tag_index,set_index,blk_offset);

//mask assign
blk_mask = 0;
set_mask = 0;
tag_mask = 0;


for(int i=1;i<=64;i++)
{
    if(i<=blk_offset)
    {

        blk_mask = (blk_mask <<1) +1;
//printf("\ntag_mask = 0x%016llx",tag_mask);
//printf("\nset_mask = %d",set_mask);

//printf("\nblk_mask = %llx",blk_mask);
    }
    else if(i>blk_offset && i<=blk_offset+set_index)
        {
            set_mask = (set_mask << 1) +blk_mask+1;

//printf("\nset_mask = %llx",set_mask);            

        }
    else if(i>set_index && i<=64)
        {
           tag_mask = (tag_mask << 1) +set_mask+blk_mask+1;

//printf("\ntag_mask = %llx",tag_mask);  


        }

}

printf("\ntag_mask = 0x%016llx",tag_mask);
printf("\nset_mask = 0x%016llx",set_mask);
printf("\nblk_mask = 0x%016llx\n",blk_mask);

printf("\ntag = 0x%016llx",tag);
printf("\nset = 0x%016llx",set);
printf("\nblk = 0x%016llx\n",blk);


//create cache memory as a Global Variable
cache_blk **mem;
mem = (cache_blk**) malloc(n_sets*sizeof(cache_blk*));
     for (int i = 0; i < n_sets; i++)
         mem[i] = (cache_blk*)malloc(assoc * sizeof(cache_blk));

//cache_blk mem[2048][4];
string fname;
string dir="D:\\Code_Practice\\C++\\Cache_Replacement\\614_Hw2_tracefiles\\";
string full_name;
ifstream flist("D:\\Code_Practice\\C++\\Cache_Replacement\\614_Hw2_tracefiles\\fn.txt");

cout<<"\nFilename,r_hit,r_miss,w_hit,w_miss,total_access,miss_rate%,hit_rate%";





 while (getline(flist, fname))
{       
      
r_hit =0;
r_miss =0;
w_hit =0;
w_miss =0;
n_pref =0;

//printf("\nSizes%d,%d\n",n_sets,assoc);


for(int i=0;i<=n_sets-1;i++)
{

for(int j=0;j<=assoc-1;j++)
{

mem[i][j].lru_value=0;
mem[i][j].c_valid=0;
mem[i][j].c_value=0;

//printf("\n lru=%d -- c_valid = %d -- c_value=%d",mem[i][j].lru_value,mem[i][j].c_valid, mem[i][j].c_value); 


}

}

//lru_status(mem);


string tmpLine;//"w 7ff0005b8";
char mode;
int count=0;

 






full_name="D:\\Code_Practice\\C++\\Cache_Replacement\\614_Hw2_tracefiles\\";
full_name.append(fname);

ifstream MyReadFile(full_name);

cout<<"\n"<<fname;
 while (getline(MyReadFile, tmpLine))
    {
        /* Get the mode and addr from cmd line */
        stringstream ss1(tmpLine);
        ss1 >> mode;
        ss1 >>  hex >> r_addr;





cache_op(r_addr,mode,rand(),mem);
cache_prefetch(Np,r_addr,mem);


ss1.clear ();
//count++;
//if(count==10)
//	break;

}
MyReadFile.close();

int total_access = r_hit + r_miss + w_hit + w_miss;


printf(",%d",r_hit);
printf(",%d",r_miss);
printf(",%d",w_hit);
printf(",%d",w_miss);

printf(",%d",total_access);

printf(",%f",100*( (r_miss + w_miss)/(float)total_access)   );
printf(",%f",100*( (r_hit + w_hit)/(float)total_access ));
printf(",n_pref = %d",n_pref);
//printf("\n r_hit = %d \n",r_hit);
//printf("r_miss = %d \n",r_miss);
//printf("w_hit = %d \n",w_hit);
//printf("w_miss = %d \n",w_miss);
//
//printf("total access = %d \n",total_access);
//
//printf("\n miss_rate% = %f",100*( (r_miss + w_miss)/(float)total_access)   );
//printf("\n hit_rate% = %f",100*( (r_hit + w_hit)/(float)total_access ));


}


flist.close(); 
return 1;
}