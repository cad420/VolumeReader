//
// Created by wyz on 2021/2/8.
//

#ifndef VOLUMEREADER_VOLUME_READER_H
#define VOLUMEREADER_VOLUME_READER_H
#include<vector>
#include<queue>
#include<string>
#include<array>
#include<mutex>
#include<condition_variable>
#include<fstream>
#include<iostream>
#define DEFAULT_VALUE 0

class IVolumeReader{
public:

};

class RawVolumeReader{
public:

};

class BlockVolumeReader{
public:
    struct RawVolumeInfo{
        std::string input_file_path;
        std::string output_file_path;
        uint32_t raw_x;
        uint32_t raw_y;
        uint32_t raw_z;
        uint32_t block_length;
        uint32_t padding;
    } raw_volume_info;
    struct Block{
        std::array<uint32_t,3> index;
        std::vector<uint8_t> data;
    };
    enum BlockState{
        UnRead,
        Store,
        Token
    };
    struct BlocksManager{
        uint32_t max_cached_batch_num;
        uint32_t max_cached_block_num;
        uint32_t cur_cached_block_num;
        std::queue<Block> block_warehouse;//size limited by memory limit
        std::vector<BlockState> block_states;//size equal to total block count
    };
public:
    BlockVolumeReader()=default;
    explicit BlockVolumeReader(RawVolumeInfo raw_volume_info) noexcept;
    virtual ~BlockVolumeReader(){
        read_task.join();
    }
    bool setupRawVolumeInfo(RawVolumeInfo raw_volume_info);
    bool setupMaxMemoryLimitInGB(uint32_t memory_limit_GB);
private:
    /**
     * once read x_block_num=(raw_x+block_length-1)/block_length blocks
     */
    void read_patch_blocks();

    bool isReadReady();



    bool read_patch_enable();
public:
    /**
     * @brief just need call once for a huge volume data's read
     */
    void start_read();
    /**
     * asynchronous get block
     * if no block or block for index is not read, wait until it read
     */
    void get_block(std::vector<uint8_t>& block,std::array<uint32_t,3>& index);
    /**
     * @return if the huge volume data finish read all
     */
    bool is_read_finish();

    bool isBlockWareHouseEmpty();

    std::array<uint32_t,3> get_dim() const;
private:
    std::string input_file_path;
    std::string output_file_path;
    uint32_t raw_x;
    uint32_t raw_y;
    uint32_t raw_z;
    uint32_t block_length;
    uint32_t padding;

    uint32_t modify_x,modify_y,modify_z;//equal to dim*block_length_nopadding
    uint32_t block_length_nopadding;

    std::fstream in;
    std::array<uint32_t, 3> dim;
    uint32_t total_block_num;
    uint64_t block_byte_size;//block_length^3
    uint32_t batch_read_cnt;//total batch read number
    uint32_t batch_read_turn;//current batch read turn
    uint32_t block_num_per_batch;
    uint64_t batch_cached_byte_size;//equal block_byte_size*block_num_per_batch


    uint32_t max_memory_size_GB;

    std::mutex batch_blocks_mutex;
    std::mutex batch_blocks_get_mutex;
    std::mutex batch_blocks_read_mutex;
    std::unique_lock<std::mutex> batch_blocks_lock;
    std::unique_lock<std::mutex> batch_blocks_get_lock;
    std::unique_lock<std::mutex> batch_blocks_read_lock;
    std::condition_variable asyn_read;
    std::condition_variable asyn_get;
    BlocksManager block_manager;
    std::thread read_task;
};

inline BlockVolumeReader::BlockVolumeReader(BlockVolumeReader::RawVolumeInfo raw_volume_info) noexcept
{
    //todo
    this->max_memory_size_GB=32;
    setupRawVolumeInfo(raw_volume_info);

}
inline bool BlockVolumeReader::setupRawVolumeInfo(BlockVolumeReader::RawVolumeInfo raw_volume_info)
{
    in.open(raw_volume_info.input_file_path.c_str(),std::ios::in|std::ios::binary);
    if(!in.is_open()){
        std::cout<<"raw volume file open failed!"<<std::endl;
        return false;
    }

    {
        input_file_path=raw_volume_info.input_file_path;
        output_file_path=raw_volume_info.output_file_path;
        raw_x=raw_volume_info.raw_x;
        raw_y=raw_volume_info.raw_y;
        raw_z=raw_volume_info.raw_z;
        block_length=raw_volume_info.block_length;
        padding=raw_volume_info.padding;

        block_length_nopadding=block_length-2*padding;
        this->dim[0]=(raw_x+block_length_nopadding-1)/block_length_nopadding;
        this->dim[1]=(raw_y+block_length_nopadding-1)/block_length_nopadding;
        this->dim[2]=(raw_z+block_length_nopadding-1)/block_length_nopadding;
        this->modify_x=this->dim[0]*block_length_nopadding;
        this->modify_y=this->dim[1]*block_length_nopadding;
        this->modify_z=this->dim[2]*block_length_nopadding;
        if(dim[0]==1 || dim[1]==1 || dim[2]==1){
            std::cout<<"bad block length!"<<std::endl;
            return false;
        }

        this->total_block_num=this->dim[0]*this->dim[1]*this->dim[2];
        //consider block_length with padding
        this->block_byte_size=block_length*block_length*block_length;
        this->batch_read_cnt=this->dim[1]*this->dim[2];
        this->batch_read_turn=0;
        this->block_num_per_batch=this->dim[0];
        this->batch_cached_byte_size=this->block_num_per_batch*this->block_byte_size;
        
        this->block_manager.max_cached_batch_num=uint64_t(this->max_memory_size_GB)*1024*1024*1024/this->batch_cached_byte_size;
        this->block_manager.max_cached_block_num=this->block_manager.max_cached_batch_num*this->block_num_per_batch;
        this->block_manager.block_states.assign(this->total_block_num,UnRead);
        this->block_manager.cur_cached_block_num=0;
    }

//    this->raw_volume_info=raw_volume_info;
    return true;
}
inline void BlockVolumeReader::start_read()
{
    std::cout<<__FUNCTION__ <<std::endl;
    if(!isReadReady()){
        std::cout<<"read is not read! check and reset raw volume info!"<<std::endl;
        return;
    }
    batch_blocks_read_lock=std::unique_lock<std::mutex>(batch_blocks_read_mutex);
    batch_blocks_get_lock=std::unique_lock<std::mutex>(batch_blocks_get_mutex);
//    batch_blocks_lock=std::unique_lock<std::mutex>(batch_blocks_mutex);
    read_task=std::thread([this](){
          std::cout<<"start read task"<<std::endl;
          while(!is_read_finish()){
              asyn_read.wait(batch_blocks_read_lock,[this]{//after wait locked, if wait then unlocked and sleeping wait
                  if(read_patch_enable())
                      return true;
                  else return false;
              });

              read_patch_blocks();
          }
          std::cout<<"read finished"<<std::endl;
          asyn_get.notify_all();
    }
    );
}

inline bool BlockVolumeReader::is_read_finish()
{
    if(batch_read_turn==batch_read_cnt){
//        std::cout<<"read finished"<<std::endl;
        return true;
    }
    else{
        return false;
    }
}

inline void BlockVolumeReader::get_block(std::vector<uint8_t> &block, std::array<uint32_t, 3>& index)
{
//    std::cout<<__FUNCTION__ <<std::endl;

    if(!is_read_finish())
        asyn_get.wait(batch_blocks_get_lock);//must wait for signal

    batch_blocks_mutex.lock();

    std::cout<<"size "<<this->block_manager.block_warehouse.size()<<std::endl;
    index=this->block_manager.block_warehouse.front().index;
    block=std::move(this->block_manager.block_warehouse.front()).data;
    this->block_manager.block_warehouse.pop();


    this->block_manager.cur_cached_block_num--;
    std::cout<<"remain "<<this->block_manager.cur_cached_block_num<<std::endl;
    batch_blocks_mutex.unlock();

    if(!is_read_finish() || read_patch_enable()){
        asyn_read.notify_one();
    }

}

inline void BlockVolumeReader::read_patch_blocks()
{
    std::cout<<__FUNCTION__ <<std::endl;

    batch_blocks_mutex.lock();


    std::vector<uint8_t> read_buffer;

    uint64_t read_batch_size=block_length*block_length*(modify_x+2*padding);
    read_buffer.assign(read_batch_size,DEFAULT_VALUE);
    uint64_t batch_slice_line_size=modify_x+2*padding;

    uint32_t z=this->batch_read_turn/this->dim[1];
    uint32_t y=this->batch_read_turn%this->dim[1];
    uint64_t batch_slice_read_num=0;
    uint64_t batch_slice_line_read_num=0;
    uint64_t batch_read_pos=0;
    uint64_t batch_slice_size=block_length*(modify_x+2*padding);
    uint64_t raw_slice_size=raw_x*raw_y;
    uint64_t y_offset,z_offset;

    if(z==0){
        batch_slice_read_num=block_length_nopadding+padding;
        z_offset=padding;
        if(y==0){
            y_offset=padding;
            batch_read_pos=0;
            batch_slice_line_read_num=block_length_nopadding+padding;
        }
        else if(y==(dim[1]-1)){
            y_offset=0;
            batch_read_pos=(uint64_t)(y*block_length_nopadding-padding)*raw_x;
            batch_slice_line_read_num=raw_y-y*block_length_nopadding+padding;
        }
        else{
            y_offset=0;
            batch_read_pos=(uint64_t)(y*block_length_nopadding-padding)*raw_x;
            batch_slice_line_read_num=block_length_nopadding+2*padding;
        }
    }
    else if(z==(dim[2]-1)){
        z_offset=0;
        batch_slice_read_num=raw_z-z*block_length_nopadding+padding;
        if(y==0){
            y_offset=padding;
            batch_read_pos=(uint64_t)(z*block_length_nopadding-padding)*raw_slice_size;
            batch_slice_line_read_num=block_length_nopadding+padding;
        }
        else if(y==(dim[1]-1)){
            y_offset=0;
            batch_read_pos=(uint64_t)(z*block_length_nopadding-padding)*raw_slice_size
                    +(y*block_length_nopadding-padding)*raw_x;
            batch_slice_line_read_num=raw_y-y*block_length_nopadding+padding;
        }
        else{
            y_offset=0;
            batch_read_pos=(uint64_t)(z*block_length_nopadding-padding)*raw_slice_size
                             +(y*block_length_nopadding-padding)*raw_x;
            batch_slice_line_read_num=block_length_nopadding+2*padding;
        }
    }
    else{
        z_offset=0;
        batch_slice_read_num=block_length;
        if(y==0){
            y_offset=padding;
            batch_read_pos=(uint64_t)(z*block_length_nopadding-padding)*raw_slice_size;
            batch_slice_line_read_num=block_length_nopadding+padding;
        }
        else if(y==(dim[1]-1)){
            y_offset=0;
            batch_read_pos=(uint64_t)(z*block_length_nopadding-padding)*raw_slice_size
                             +(y*block_length_nopadding-padding)*raw_x;
            batch_slice_line_read_num=raw_y-y*block_length_nopadding+padding;
        }
        else{
            y_offset=0;
            batch_read_pos=(uint64_t)(z*block_length_nopadding-padding)*raw_slice_size
                             +(y*block_length_nopadding-padding)*raw_x;
            batch_slice_line_read_num=block_length_nopadding+2*padding;
        }
    }

    for(uint64_t i=0;i<batch_slice_read_num;i++){
        in.seekg(batch_read_pos+i*raw_slice_size,std::ios::beg);

        std::vector<uint8_t> read_batch_slice_buffer;
        read_batch_slice_buffer.assign(raw_x*batch_slice_line_read_num,DEFAULT_VALUE);

        in.read(reinterpret_cast<char*>(read_batch_slice_buffer.data()),read_batch_slice_buffer.size());

        for(uint64_t j=0;j<batch_slice_line_read_num;j++) {
            uint64_t offset=(z_offset+i)*(modify_x+2*padding)*block_length+(y_offset+j)*(modify_x+2*padding)+padding;
            memcpy(read_buffer.data()+offset,read_batch_slice_buffer.data()+j*raw_x,raw_x);
        }

//        for(uint32_t j=0;j<batch_slice_line_read_num;j++){
//            std::vector<uint8_t> read_line_buffer;
//            read_line_buffer.assign(batch_slice_line_size,DEFAULT_VALUE);
//            in.read(reinterpret_cast<char*>(read_line_buffer.data()),raw_x);
//            uint32_t offset=(i+z_offset)*(modify_x+2*padding)*block_length+(y_offset+j)*(modify_x+2*padding)+padding;
//            memcpy(read_buffer.data()+offset,read_line_buffer.data(),read_line_buffer.size());
//        }

    }

    uint64_t batch_length=modify_x+2*padding;
    for(uint32_t i=0;i<this->block_num_per_batch;i++){
        Block block;
        block.index={i,y,z};
        block.data.assign(block_byte_size,DEFAULT_VALUE);
        for(size_t z_i=0;z_i<block_length;z_i++){
            for(size_t y_i=0;y_i<block_length;y_i++){
                uint64_t offset=z_i*batch_slice_size+y_i*batch_length+i*block_length_nopadding;
                memcpy(block.data.data()+z_i*block_length*block_length+y_i*block_length,
                       read_buffer.data()+offset,block_length);
            }
        }
        this->block_manager.cur_cached_block_num++;
        this->block_manager.block_warehouse.push(std::move(block));

    }

    this->batch_read_turn++;

    read_buffer.clear();
    std::cout<<"finish read batch"<<std::endl;
    batch_blocks_mutex.unlock();

    asyn_get.notify_all();
    asyn_read.notify_one();

}

inline bool BlockVolumeReader::isReadReady()
{

    return true;
}

bool BlockVolumeReader::read_patch_enable()
{
    if(this->block_manager.cur_cached_block_num+this->block_num_per_batch<=this->block_manager.max_cached_block_num){
//        std::cout<<"read enable"<<std::endl;
        return true;
    }
    else{
//        std::cout<<"read disable"<<std::endl;
        return false;
    }
}

inline bool BlockVolumeReader::isBlockWareHouseEmpty() {
    return this->block_manager.cur_cached_block_num==0;
}

std::array<uint32_t, 3> BlockVolumeReader::get_dim() const {
    return this->dim;
}


#endif //VOLUMEREADER_VOLUME_READER_H
