//
// Created by wyz on 2021/2/8.
//

#include<VolumeReader/volume_reader.h>
#define volume_name0 "aneurism_256_256_256_uint8.raw"
#define volume_name1 "raw_256_256_256_uint8.raw"
int main(int argc,char** argv)
{

    BlockVolumeReader reader;
    reader.setupRawVolumeInfo({volume_name0,"out.h264",256,256,256,256,1});
    reader.start_read();
    std::vector<uint8_t> data;
    int cnt=0;
    std::string out_dir="./raw_block_test/";
    while(!reader.is_read_finish() || !reader.isBlockWareHouseEmpty()){
        data.clear();
        std::cout<<"turn "<<cnt++<<std::endl;
        std::array<uint32_t,3> idx={};
        reader.get_block(data,idx);
//        std::cout<<data.size()<<std::endl;
        std::cout<<idx[0]<<" "<<idx[1]<<" "<<idx[2]<<std::endl;

        std::string out_name=std::to_string(idx[0])+"#"+std::to_string(idx[1])+"#"+std::to_string(idx[2])+"_256_256_256_uint8.raw";
        std::fstream out(out_dir+out_name,std::ios::out|std::ios::binary);
        out.write(reinterpret_cast<char*>(data.data()),data.size());
        out.close();
    }
    return 0;
}
