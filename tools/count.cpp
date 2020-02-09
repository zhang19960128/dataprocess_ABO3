#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <string>
#include <cmath>
#include <list>
double average(std::list<double>& listA){
  double sum=0.0;
  int count=0;
  for(std::list<double>::iterator a=listA.begin();a!=listA.end();a++){
    sum=sum+*a;
    count=count+1;
  }
  if(count=0){
    return 0.0;
  }
  else{
    return sum/count;
  }
}
int next(int i,int direction,int period){
  if(direction==0){
    return (i+1)%period;
  }
  else if(direction==1){
    return (i+period)%(period*period);
  }
  else if(direction==2){
    return (i+period*period)%(period*period*period);
  }
}
int main(){
  MPI_Init(NULL,NULL);
  int simulationtime=100000/200;
  int cell=20;
  int world_rank,world_size;
  MPI_Comm_size(MPI_COMM_WORLD,&world_size);
  MPI_Comm_rank(MPI_COMM_WORLD,&world_rank);
  MPI_File mpifile;
  MPI_Status status;
  MPI_Offset offset;
  std::string filename="local_polar.bin";
  double* localpolar=new double[3];
  std::list<double> px,py,pz;
  MPI_File_open(MPI_COMM_WORLD,filename.c_str(),MPI_MODE_RDONLY,MPI_INFO_NULL,&mpifile);
  double* polaraverage=new double [3*cell*cell*cell];
  for(size_t i=0;i<3*cell*cell*cell;i++){
    polaraverage[i]=0.0;
  }
  for(size_t loop=world_rank;loop<cell*cell*cell;loop=loop+world_size){
    px.clear();
    py.clear();
    pz.clear();
    for(size_t frame=0;frame<simulationtime;frame++){
      offset=(frame*(3*cell*cell*cell)+loop*3)*sizeof(double);
      MPI_File_read_at(mpifile,offset,localpolar,3,MPI::DOUBLE,&status);
      std::cout<<localpolar[0]<<" "<<localpolar[1]<<" "<<localpolar[2]<<std::endl;
      px.push_back(localpolar[0]);
      py.push_back(localpolar[1]);
      pz.push_back(localpolar[2]);
    }
    polaraverage[loop*3+0]=average(px);
    polaraverage[loop*3+1]=average(py);
    polaraverage[loop*3+2]=average(pz);
  }
  double* reducepolar=new double [3*cell*cell*cell];
  int* domainsizecount=new int [cell];
  int* domainsizecount_reduce=new int [cell];
  MPI_Allreduce(polaraverage,reducepolar,3*cell*cell*cell,MPI::DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  /*starting from X direction*/
  int nx,ny,nz,nindex;
  nx=0;
  size_t record_start;
  double decay_rate=0.7;
  for(size_t i=world_rank;i<cell*cell;i=i+world_size){
    ny=i%cell;
    nz=(i-ny)/cell;
    px.clear();
    for(size_t j=0;j<cell;j++){
     nindex=j+ny*cell+nz*cell*cell;
     if(px.size()==0){
      px.push_back(reducepolar[0+nindex*3]);
     }
     else{
      if(std::fabs(average(px)-reducepolar[0+nindex*3])/std::fabs(average(px))>decay_rate){
        record_start=j;
        break;
      }
     }
    }
    px.clear();
    for(size_t j=record_start;j<cell+record_start;j++){
      nindex=j%cell+ny*cell+nz*cell*cell;
      if(px.size()==0){
        px.push_back(reducepolar[0+nindex*3]);
      }
      else{
        if(std::fabs(average(px)-reducepolar[0+nindex*3])/std::fabs(average(px))>decay_rate){
          domainsizecount[px.size()]=domainsizecount[px.size()]+1;
          px.clear();
        }
        else{
          px.push_back(reducepolar[0+nindex*3]);
        }
      }
    }
  }
  MPI_Allreduce(domainsizecount,domainsizecount_reduce,cell,MPI::INT,MPI_SUM,MPI_COMM_WORLD);
  if(world_rank==0){
    for(size_t i=0;i<cell;i++){
      std::cout<<i<<" "<<domainsizecount[i]<<std::endl;
    }
  }
  MPI_File_close(&mpifile);
  delete [] localpolar;
  delete [] polaraverage;
  delete [] reducepolar;
  px.clear();
  py.clear();
  pz.clear();
  MPI_Finalize();
}
