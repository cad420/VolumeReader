//
// Created by wyz on 2021/2/8.
//

#include<VolumeReader/volume_reader.h>

int main(int argc,char** argv)
{

    BlockVolumeReader reader;
    reader.setupRawVolumeInfo({"aneurism_256_256_256_uint8.raw","out.h264",256,256,256,32,1});
    reader.start_read();
    std::vector<uint8_t> data;
    int cnt=0;
//    while(!reader.is_read_finish() || !reader.isBlockWareHouseEmpty()){
//        std::cout<<"turn "<<cnt++<<std::endl;
//        std::array<uint32_t,3> idx={};
//        reader.get_block(data,idx);
////        std::cout<<data.size()<<std::endl;
////        std::cout<<idx[0]<<" "<<idx[1]<<" "<<idx[2]<<std::endl;
//    }
    return 0;
}
