#include <nmmintrin.h>
#include <stdio.h>
#include <omp.h>
#define KERNX 3 //this is the x-size of the kernel. It will always be odd.
#define KERNY 3 //this is the y-size of the kernel. It will always be odd.
int conv2D(float* in, float* out, int data_size_X, int data_size_Y,
                    float* kernel)
{
	
    // the x coordinate of the kernel's center
	int kern_cent_X = (KERNX - 1)/2;
    // the y coordinate of the kernel's center
    int kern_cent_Y = (KERNY - 1)/2;
	
    int padded_size_X = data_size_X + 2, padded_size_Y = data_size_Y + 2;
	int padded_size = padded_size_X * padded_size_Y;
	float* padded_in = malloc(padded_size_X*padded_size_Y * sizeof(float));
	
	
	__m128 pad_row = _mm_setzero_ps();
	int xx = 0;
	float* start_of_last_row = padded_in + padded_size - padded_size_X;
	for(; xx < data_size_X -1; xx+=4){
		_mm_storeu_ps(padded_in + xx, pad_row);
		_mm_storeu_ps(start_of_last_row + xx, pad_row);
	}
	for(; xx < data_size_X + 2; xx += 1){
		padded_in[xx] = 0;
	}
	
	int i = 0, j = padded_size_X;
	for(;j < padded_size;j+=padded_size_X){
		padded_in[i] = 0;
		padded_in[j] = 0;
		i+=padded_size_X;
	}	
	


	
	#pragma omp parallel for schedule(dynamic, 8) firstprivate(pad_row)
	for(int y = 1; y <= data_size_Y; y++){
		int pad_index = y*padded_size_X + 1, in_index = y*data_size_X - data_size_X;
		int last_in_row = pad_index + data_size_X;
		for(; pad_index < last_in_row - 3; pad_index += 4){
			pad_row = _mm_loadu_ps(in+in_index);
			_mm_storeu_ps(padded_in+pad_index,pad_row);
			in_index += 4;	
		}
		for(; pad_index < last_in_row; pad_index++)
			padded_in[pad_index] = in[in_index++];
	}
   	//=================Kernel Flip=========================== 
	float flipped_kernel[9];
    for(int i = 0,j=8; i < 9; i++,j--){
		flipped_kernel[i] = kernel[j];
    }

	__m128 kernel_row = _mm_setzero_ps();
	__m128 matrix_row = _mm_setzero_ps();
	__m128 sum_res = _mm_setzero_ps();

	

	
	#pragma omp parallel for schedule(dynamic, 8) firstprivate(flipped_kernel, padded_size_X, data_size_X, data_size_Y) private(kernel_row, matrix_row, sum_res)
	for(int y = 1; y <= data_size_Y; y++){ 
		int index = y*padded_size_X + 1, out_index = y*data_size_X - data_size_X;
 		int last = index + data_size_X;
    			for(; index < last-3; index+=4){
					float* in_location = padded_in + index;
					kernel_row = _mm_load1_ps(flipped_kernel);
					matrix_row = _mm_loadu_ps(in_location - 1 - padded_size_X);
					sum_res = _mm_mul_ps(kernel_row, matrix_row);

					kernel_row = _mm_load1_ps(flipped_kernel + 1);
					matrix_row = _mm_loadu_ps(in_location - padded_size_X);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);
		
					kernel_row = _mm_load1_ps(flipped_kernel + 2);
					matrix_row = _mm_loadu_ps(in_location + 1 - padded_size_X);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);

					kernel_row = _mm_load1_ps(flipped_kernel + 3);
					matrix_row = _mm_loadu_ps(in_location -  1);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);
					
					kernel_row = _mm_load1_ps(flipped_kernel + 4);
					matrix_row = _mm_loadu_ps(in_location);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);

					kernel_row = _mm_load1_ps(flipped_kernel + 5);
					matrix_row = _mm_loadu_ps(in_location + 1);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);

					kernel_row = _mm_load1_ps(flipped_kernel + 6);
					matrix_row = _mm_loadu_ps(in_location - 1 + padded_size_X);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);
				
					kernel_row = _mm_load1_ps(flipped_kernel + 7);
					matrix_row = _mm_loadu_ps(in_location + padded_size_X);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);

					kernel_row = _mm_load1_ps(flipped_kernel + 8);
					matrix_row = _mm_loadu_ps(in_location + 1 + padded_size_X);
					matrix_row = _mm_mul_ps(kernel_row, matrix_row);
					sum_res = _mm_add_ps(sum_res, matrix_row);

					_mm_storeu_ps(out+out_index,sum_res);
					out_index += 4;
			

			}

	            for(; index < last; index++){
						out[out_index] = flipped_kernel[0] * padded_in[index - 1 - padded_size_X];
						out[out_index] += flipped_kernel[1] * padded_in[index - padded_size_X];
						out[out_index] += flipped_kernel[2] * padded_in[index + 1 - padded_size_X];
						out[out_index] += flipped_kernel[3] * padded_in[index - 1];
						out[out_index] += flipped_kernel[4] * padded_in[index];
						out[out_index] += flipped_kernel[5] * padded_in[index +1];
						out[out_index] += flipped_kernel[6] * padded_in[index - 1 + padded_size_X];
						out[out_index] += flipped_kernel[7] * padded_in[index + padded_size_X];
						out[out_index] += flipped_kernel[8] * padded_in[index + 1 + padded_size_X];
                        out_index++;
    	}
	}

	free(padded_in);	
	return 1;
}
