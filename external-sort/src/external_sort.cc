#include "moderndbs/external_sort.h"
#include "moderndbs/file.h"
#include <vector>
#include <algorithm>
namespace moderndbs {

struct node {
   size_t value;
   size_t index;
   node(size_t value_, size_t index_) : value(value_), index(index_) {}
};

void external_sort(File& input, size_t num_values, File& output, size_t mem_size) {
   // TODO: add your implementation here
   // external sort achieve by heap
   // if the input file is empty, return
   if (num_values == 0) {
      output.resize(0);
      return;
   }
   // if the input file only has one value, write it to the output file
   if (num_values == 1) {
      // read the input file block
      auto block = std::make_unique<char[]>(sizeof(size_t));
      input.read_block(0, sizeof(size_t), block.get());
      // write the block to the output file
      output.resize(sizeof(size_t));
      output.write_block(block.get(), 0, sizeof(size_t));
      return;
   }
   // // if the input file is smaller than the memory size, sort in memory
   if (num_values <= mem_size) {
      //read the input file block to a uint64_t vec
      std::vector<size_t> vec(num_values);
      input.read_block(0, num_values * sizeof(size_t), reinterpret_cast<char*>(vec.data()));
      // sort the vector
      std::sort(vec.begin(), vec.end());
      // write the vec to the output file
      output.resize(num_values * sizeof(size_t));
      output.write_block(reinterpret_cast<char*>(vec.data()), 0, num_values * sizeof(size_t));
      return;
   }
   // if the input file is bigger than the memory size, sort in external
   if (num_values >= mem_size) {
      size_t num_blocks = (num_values + mem_size - 1) / mem_size;
      output.resize(0);
      output.resize(2 * num_values * sizeof(size_t));
      // make a array to store the data in the input file
      std::vector<size_t> vec(num_values);
      // read the input file block to the array
      input.read_block(0, num_values * sizeof(size_t), reinterpret_cast<char*>(vec.data()));
      // sort every block in the input file
      for (size_t i = 0; i < num_blocks; i++) {
         if ((i + 1) * mem_size >= num_values) {
            size_t rest_block_num = num_values - i * mem_size;
            std::vector<size_t> vec(rest_block_num);
            input.read_block(i * mem_size * sizeof(size_t), rest_block_num * sizeof(size_t), reinterpret_cast<char*>(vec.data()));
            // sort the vec
            std::sort(vec.begin(), vec.end());
            // write the block to the output file
            output.write_block(reinterpret_cast<char*>(vec.data()), (num_values + i * mem_size) * sizeof(size_t), rest_block_num * sizeof(size_t));
         } else {
            std::vector<size_t> vec(mem_size);
            // read the input file block
            input.read_block(i * mem_size * sizeof(size_t), mem_size * sizeof(size_t), reinterpret_cast<char*>(vec.data()));
            // sort the vec
            std::sort(vec.begin(), vec.end());
            // write the block to the output file
            output.write_block(reinterpret_cast<char*>(vec.data()), (num_values + i * mem_size) * sizeof(size_t), mem_size * sizeof(size_t));
         }
      }
      // Above is bug free!!!!!!!!!!!!!

      // merge the sorted blocks
      // read the first value of every block

      std::vector<node> first_value_vec;
      for (size_t i = 0; i < num_blocks; i++) {
         size_t first_value;
         output.read_block((i * mem_size + num_values) * sizeof(size_t), sizeof(size_t), reinterpret_cast<char*>(&first_value));
         first_value_vec.push_back(node(first_value, (i * mem_size)));
      }
      std::make_heap(first_value_vec.begin(), first_value_vec.end(), [](node a, node b) { return a.value > b.value; });
      for (size_t i = 0; i < num_values; i++) {
         node first_node = first_value_vec.front();
         output.write_block(reinterpret_cast<char*>(&(first_node.value)), i * sizeof(size_t), sizeof(size_t));
         // pop the first value of the heap and get the node
         std::pop_heap(first_value_vec.begin(), first_value_vec.end(), [](node a, node b) { return a.value > b.value; });
         first_value_vec.pop_back();
         // if the first_node.value is not very large, push the first value of the block to the heap
         if ((first_node.index + 1) % mem_size == 0 || (first_node.index + 1) >= num_values) {
            size_t first_value = ULONG_MAX;
            first_value_vec.push_back(node(first_value, first_node.index));
            std::push_heap(first_value_vec.begin(), first_value_vec.end(), [](node a, node b) { return a.value > b.value; });
         } else {
            size_t first_value;
            output.read_block((num_values + first_node.index + 1) * sizeof(size_t), sizeof(size_t), reinterpret_cast<char*>(&first_value));
            first_value_vec.push_back(node(first_value, first_node.index + 1));
            std::push_heap(first_value_vec.begin(), first_value_vec.end(), [](node a, node b) { return a.value > b.value; });
         }
      }
      output.resize(num_values * sizeof(size_t));
      return;
   }
   return;
}

} // namespace moderndbs