#ifndef NNET_NEURON_
#define NNET_NEURON_

#include "../include/nnet.h"

#define LEAKY 0.1

enum neuron_t
{ RELU		= 1,
  SIGMOID	= 2,
  TANH		= 3
};

template <typename DT>
XPU_KERNEL(NeuronForward) (
  const int num_kernels, const DT* src_data, DT* dst_data)
{ kernel_for (i, num_kernels)
    dst_data[i] = src_data[i] * (src_data[i] >= (DT)0. ? (DT)1. : (DT)LEAKY);
};

template <typename DT>
XPU_KERNEL(NeuronBackward) (
  const int num_kernels, DT* src_diff, const DT* dst_diff)
{ kernel_for (i, num_kernels)
    src_diff[i] = dst_diff[i] * (src_diff[i] >= (DT)0. ? (DT)1. : (DT)LEAKY);
};



#ifdef __CUDACC__
template LayerNeuron<GPU>::LayerNeuron (ParaLayer &pl, const int did, TensorGPUf &src, TensorGPUf &dst);
#else
template LayerNeuron<CPU>::LayerNeuron (ParaLayer &pl, const int did, TensorCPUf &src, TensorCPUf &dst);
#endif

LAYER_FORWARD (LayerNeuron)
{ 
#ifdef __CUDACC__
  dst_.mem_set (0);
  cuda_check (cudnnActivationForward (CUDNN_HANDLE, get_activation_type(),
    &alpha, srcDesc_, src_.dptr,
    &beta,  dstDesc_, dst_.dptr));
#else
  const int N = dst_.size();
  XPU_KERNEL_LAUNCH (NeuronForward,  cuda_get_blocks(N), CUDA_NUM_THREADS, 0, CUDNN_STREAM,
    N, src_.dptr, dst_.dptr);
  cuda_sync_check ("NeuronForward");
#endif
}

LAYER_BACKPROP (LayerNeuron)
{
#ifdef __CUDACC__
  cuda_check (cudnnActivationBackward (CUDNN_HANDLE, get_activation_type(),
    &alpha, dstDesc_, src_.dptr, dstDesc_, dst_.dptr, srcDesc_, src_.dptr,
    &beta,  srcDesc_, dst_.dptr));
  src_.copy (dst_);
#else
  const int N = dst_.size();
  if (is_prop_grad)
  XPU_KERNEL_LAUNCH (NeuronBackward, cuda_get_blocks(N), CUDA_NUM_THREADS, 0, CUDNN_STREAM,
    N, src_.dptr, dst_.dptr);
  cuda_sync_check ("NeuronBackward");
#endif
}

LAYER_INIT (LayerNeuron)
{ dst_.create (src_.shape, did_);
#ifdef __CUDACC__
  cuda_check (cudnnCreateTensorDescriptor (&srcDesc_));
  cuda_check (cudnnCreateTensorDescriptor (&dstDesc_));

  src_.setTensor4dDesc (srcDesc_);
  dst_.setTensor4dDesc (dstDesc_);
#endif
}

template <typename XPU>
cudnnActivationMode_t LayerNeuron<XPU>::get_activation_type ()
{ if   (pl_.neuron == SIGMOID) return CUDNN_ACTIVATION_SIGMOID;
  else if (pl_.neuron == TANH) return CUDNN_ACTIVATION_TANH;
  else                         return CUDNN_ACTIVATION_RELU;
}

#endif
