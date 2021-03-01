//
// Created by wyz on 2021/2/11.
//

#include<vector>
#include<fstream>
#include<iostream>
#include<string>
#include<cstring>
void gen_raw()
{
    std::cout<<"raw gen"<<std::endl;
    uint32_t raw_x,raw_y,raw_z;
    raw_x=raw_y=raw_z=256;
    std::vector<uint8_t> data;
    data.resize(uint64_t(raw_x)*raw_y*raw_z);
    for(size_t i=0;i<data.size();i++){
        data[i]=i%256;
    }
    std::string out_name="raw_"+std::to_string(raw_x)+"_"+std::to_string(raw_y)+"_"+std::to_string(raw_z)+"_uint8.raw";
    std::fstream out(out_name,std::ios::out|std::ios::binary);
    out.write(reinterpret_cast<char*>(data.data()),data.size());
    out.close();
}
void gen_inspect()
{
    std::cout<<"gen inspect"<<std::endl;
    std::string in_file_path="./raw_block_test/0_0_0";
    std::fstream in(in_file_path,std::ios::in|std::ios::binary);
    if(!in.is_open()){
        std::cout<<"file open failed!"<<std::endl;
        return;
    }
    std::vector<uint8_t> data;
    data.resize(64*64*64);
    in.read(reinterpret_cast<char*>(data.data()),data.size());
    for(size_t i=0;i<data.size();i++){
        if(i%64==0)
            std::cout<<std::endl;
        std::cout<<(int)data[i]<<" ";
    }
}
void read_raw()
{
    std::string file_name="uncompress_test.raw";
    std::fstream in(file_name,std::ios::in|std::ios::binary);
    if(!in.is_open()){
        std::cout<<"file open failed!"<<std::endl;
        return;
    }
    std::vector<uint8_t> data;
    size_t length=256;
    data.resize(length*length*length);
    in.read(reinterpret_cast<char*>(data.data()),data.size());
    for(size_t z=0;z<length;z++){
        for(size_t y=0;y<length;y++){
            for(size_t x=0;x<length;x++){
                size_t index=z*length*length+y*length+x;
                if( index%4==0 && (int)data[index]!=0){

                    std::cout<<"index: "<<index<<"\terror: "<<(int)data[index]<<std::endl;
                }
//                if(z==0){
//                    std::cout<<(int)data[index]<<" ";
//                }
            }
//            std::cout<<std::endl;
        }
    }
}
int main(int argc,char** argv)
{
    read_raw();
    return 0;
}