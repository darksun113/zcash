#include "uint252.h"

template<typename FieldT>
pb_variable_array<FieldT> from_bits(std::vector<bool> bits, pb_variable<FieldT>& ZERO) {
    pb_variable_array<FieldT> acc;

    BOOST_FOREACH(bool bit, bits) {
        acc.emplace_back(bit ? ONE : ZERO);
    }

    return acc;
}

std::vector<bool> trailing252(std::vector<bool> input) {
    if (input.size() != 256) {
        throw std::length_error("trailing252 input invalid length");
    }

    return std::vector<bool>(input.begin() + 4, input.end());
}

template<typename T>
std::vector<bool> to_bool_vector(T input) {
    std::vector<unsigned char> input_v(input.begin(), input.end());

    return convertBytesVectorToVector(input_v);
}

std::vector<bool> uint256_to_bool_vector(uint256 input) {
    return to_bool_vector(input);
}

std::vector<bool> uint252_to_bool_vector(uint252 input) {
    return trailing252(to_bool_vector(input));
}

std::vector<bool> uint64_to_bool_vector(uint64_t input) {
    auto num_bv = convertIntToVectorLE(input);
    
    return convertBytesVectorToVector(num_bv);
}

void insert_uint256(std::vector<bool>& into, uint256 from) {
    std::vector<bool> blob = uint256_to_bool_vector(from);
    into.insert(into.end(), blob.begin(), blob.end());
}

void insert_uint64(std::vector<bool>& into, uint64_t from) {
    std::vector<bool> num = uint64_to_bool_vector(from);
    into.insert(into.end(), num.begin(), num.end());
}

template<typename T>
T swap_endianness_u64(T v) {
    if (v.size() != 64) {
        throw std::length_error("invalid bit length for 64-bit unsigned integer");
    }

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 8; j++) {
            std::swap(v[i*8 + j], v[((7-i)*8)+j]);
        }
    }

    return v;
}

template<typename FieldT>
linear_combination<FieldT> packed_addition(pb_variable_array<FieldT> input) {
    auto input_swapped = swap_endianness_u64(input);
    

    return pb_packing_sum<FieldT>(pb_variable_array<FieldT>(
        input_swapped.rbegin(), input_swapped.rend()
    ));
}

template<typename FieldT>
linear_combination<FieldT> packed_true_value(pb_variable_array<FieldT> input, protoboard<FieldT> pb) {
    bit_vector vpub_old_bits = input.get_bits(pb);
    //vpub_old_bits[0]=vpub_old_bits[1]=1;
    //Added by Kelvin, 20181029 - Clear color bits (8 bits)
    for(size_t i = 0; i < 8; i++) {
        vpub_old_bits[i] = 0;
    }
 
    input.fill_with_bits(pb, vpub_old_bits);
    auto input_swapped = swap_endianness_u64(input);

    return pb_packing_sum<FieldT>(pb_variable_array<FieldT>(
        input_swapped.rbegin(), input_swapped.rend()
    ));
}

template<typename FieldT>
pb_variable_array<FieldT> fetch_color(pb_variable_array<FieldT> input, protoboard<FieldT> pb) {
    //Added by Kelvin, 20181101 - Clear value bits (56 bits)
    bit_vector vpub_old_bits = input.get_bits(pb);
    for(size_t i = 8; i < 64; i++) {
        vpub_old_bits[i] = 0;
    }
    vpub_old_bits[0] = vpub_old_bits[1] = 1;
    input.fill_with_bits(pb, vpub_old_bits);
    auto input_swapped = swap_endianness_u64(input);

    return pb_variable_array<FieldT>(
        input_swapped.rbegin(), input_swapped.rend()
    );
}

template<typename FieldT>
pb_variable_array<FieldT> pb_variable_array_and(pb_variable_array<FieldT> input1, pb_variable_array<FieldT> input2, protoboard<FieldT> pb) {
    //Added by Kelvin, 20181101 - Clear value bits (56 bits)
    bit_vector bits1 = input1.get_bits(pb);
    bit_vector bits2 = input2.get_bits(pb);

    for(size_t i = 0; i < 64; i++) {
        bits1[i] = bits1[i] & bits2[i];
    }

    input1.fill_with_bits(pb, bits1);
    auto input_swapped = swap_endianness_u64(input1);

    return pb_variable_array<FieldT>(
        input_swapped.rbegin(), input_swapped.rend()
    );
}

template<typename FieldT>
linear_combination<FieldT> packed_color(pb_variable_array<FieldT> input, protoboard<FieldT> pb) {
    //Added by Kelvin, 20181029 - Clear value bits (56 bits)
    bit_vector vpub_old_bits = input.get_bits(pb);
    for(size_t i = 8; i < 64; i++) {
        vpub_old_bits[i] = 0;
    }
    vpub_old_bits[0] = vpub_old_bits[1] = 1;
    /*LogPrintf("%u: Old: 0x", index);
    for(size_t i = 0; i < 64; i++) {
        if(vpub_old_bits[i])
            LogPrintf("1");
        else
            LogPrintf("0");
    }
    LogPrintf("\n");   */
   
    input.fill_with_bits(pb, vpub_old_bits);
    auto input_swapped = swap_endianness_u64(input);


    pb_variable_array<FieldT> temp = pb_variable_array<FieldT>(
        input_swapped.rbegin(), input_swapped.rend());

    bit_vector bits = temp.get_bits(pb);
    LogPrintf("Old_reversed: 0x");
    for(size_t i = 0; i < 64; i++) {
        if(bits[i])
            LogPrintf("1");
         else
            LogPrintf("0");
    }
    LogPrintf("\n");

    linear_combination<FieldT> ret = pb_packing_sum<FieldT>(temp);

    auto it1 = ret.terms.begin();
    LogPrintf("sizeof(): %d\n", sizeof(it1->coeff));
    LogPrintf("Old_reversed_linear_combination: 0x");
    while(it1!=ret.terms.end()) {
        if(it1->coeff.is_zero())
            LogPrintf("0");
        else
            LogPrintf("1");
        it1++;
    }
    LogPrintf("\n");

    return ret; 
}