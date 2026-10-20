__global__ void adaptive_fixture_kernel(float* value) { value[threadIdx.x] += 1.0f; }
