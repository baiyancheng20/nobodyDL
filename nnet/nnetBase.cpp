#ifndef NNET_BASE_
#define NNET_BASE_

#include "../include/nnet.h"

#ifndef __CUDACC__
string ParaLayer::get_layer_type ()
{ switch (type)
  { case kConvolution	: return "Convolution";
    case kDropout	: return "Dropout\t\t" + std::to_string(drop);
    case kLoss		: return "Loss";
    case kNeuron	: return "Neuron";
    case kPooling	: return "Pooling\t\t" + std::to_string(ksize);
    default		: LOG (FATAL) << "unknown layer type";
  }
}

void ParaLayer::get_layer_info ()
{ LOG (INFO) << "\tLayer initializing\t" << get_layer_type();
}

void ParaLayer::get_model_info ()
{ char pszstr[16];  sprintf (pszstr, "\t%.2f", sigma);
  LOG (INFO) << "\tModel initialized\t"  << get_layer_type() << pszstr;
}



int ParaNNet::get_layer_type (const char *t)
{ if (!strcmp (t, "conv"	)) return kConvolution;
  if (!strcmp (t, "dropout"	)) return kDropout;
  if (!strcmp (t, "loss"	)) return kLoss;
  if (!strcmp (t, "neuron"	)) return kNeuron;
  if (!strcmp (t, "pool"	)) return kPooling;
  LOG (FATAL) << "unknown layer type";
  return 0;
}

void ParaNNet::config (const libconfig::Config &cfg)
{ min_device = cfg.lookup ("model.min_device");
  max_device = cfg.lookup ("model.max_device");
  num_device = max_device - min_device + 1;
  num_nnets  = max_device + 1;
  num_evals  = cfg.lookup ("model.num_evals");
  num_evals /= num_device;

  stt_round  = cfg.lookup ("model.stt_round");
  end_round  = cfg.lookup ("model.end_round");
  max_round  = cfg.lookup ("model.max_round");

  tFormat_	= TensorFormat (cfg);  // TODO
  train_	= ParaFileData (cfg, "traindata");
  predt_	= ParaFileData (cfg,  "testdata");
  dataType	= train_.type;

  using namespace libconfig;
  Setting
  &layer_type	= cfg.lookup ("layer.type"),
  &ksize	= cfg.lookup ("layer.ksize"),
  &pad		= cfg.lookup ("layer.pad"),
  &stride	= cfg.lookup ("layer.stride"),
  &flts		= cfg.lookup ("layer.flts"),
  &neuron	= cfg.lookup ("layer.neuron_t"),
  &pool		= cfg.lookup ("layer.pool_t"),
  &drop		= cfg.lookup ("layer.dropout"),
  &loss		= cfg.lookup ("layer.loss_t");

  Setting
  &isLoad	= cfg.lookup ("model.isLoad"),
  &isFixed	= cfg.lookup ("model.isFixed"),
  &sigma	= cfg.lookup ("model.sigma");

  float epsW	= cfg.lookup ("optim.epsW");
  float epsB	= cfg.lookup ("optim.epsB");
  float epsE	= cfg.lookup ("optim.epsE");
  float wd	= cfg.lookup ("optim.wd");

  paraLayer_.clear();
  for (int i = 0, j = 0, idxn = 0; i < layer_type.getLength(); i++)
  { ParaLayer pl;
    pl.type	= get_layer_type (layer_type[i]);
    pl.idxs	= idxn;
    pl.idxd	= idxn+1;
    pl.ksize	= ksize[i];
    pl.pad	= pad[i];
    pl.stride	= stride[i];
    pl.flts	= flts[i];

    pl.neuron	= neuron[i];
    pl.pool	= pool[i];
    pl.drop	= drop[i];
    pl.loss	= loss[i];

    if (pl.type == kConvolution)
    { pl.isLoad	= isLoad[j];
      pl.isFixed= isFixed[j];
      pl.sigma	= sigma[j];
      j++;
    }

    paraLayer_.push_back (pl);
    idxn++;

    if (pl.neuron > 0)
    { pl.type	= kNeuron;
      pl.idxs	= idxn;
      pl.idxd	= idxn+1;
      paraLayer_.push_back (pl);
      idxn++;
    }
    if (pl.drop > 0.)
    { pl.type	= kDropout;
      pl.idxs	= idxn;
      pl.idxd	= idxn+1;
      paraLayer_.push_back (pl);
      idxn++;
    }
  }

  paraWmat_.clear();
  paraBias_.clear();
  for (int i = 0; i < isLoad.getLength(); i++)
  { const float lr_multi = num_device * tFormat_.nums / 128.f;
    ParaOptim po;
    po.type	= po.get_optim_type (cfg.lookup ("optim.type"));
    po.algo	=                    cfg.lookup ("optim.algo");
    po.isFixed	= isFixed[i];

    po.lr_last	= epsE;  po.lr_last *= lr_multi;
    po.lr_base	= epsW;  po.lr_base *= lr_multi;
    po.wd_base	= wd;
    paraWmat_.push_back (po);

    po.lr_base	= epsB;  po.lr_base *= lr_multi;
    po.wd_base	= 0.f;
    paraBias_.push_back (po);
  }

  model_.set_para (cfg);

  num_layers = paraLayer_.size();
  num_optims = paraWmat_ .size();  // TODO
  num_nodes  = 0;
  for (int i = 0; i < num_layers; ++i)  // TODO
    num_nodes = std::max (paraLayer_[i].idxd + 1, num_nodes);
}
#endif

#ifdef __CUDACC__
template <>
void TensorGPUf::setTensor4dDesc (cudnnTensorDescriptor_t &desc)
{ cuda_check (cudnnSetTensor4dDescriptor (desc, CUDNN_TENSOR_NCHW, CUDNN_DATA_FLOAT, nums(), chls(), rows(), cols()));
}

template <>
void TensorGPUf::setFilter4dDesc (cudnnFilterDescriptor_t &desc)
{ cuda_check (cudnnSetFilter4dDescriptor (desc, CUDNN_DATA_FLOAT, nums(), chls(), rows(), cols()));
}
#endif

template<typename XPU>
LayerBase<XPU>* create_layer (ParaLayer &pl, const int did, Tensor<XPU, float> &src, Tensor<XPU, float> &dst)
{ pl.get_layer_info ();
  switch (pl.type)
  { case kConvolution	: return new LayerConvolution<XPU>	(pl, did, src, dst);
    case kDropout	: return new LayerDropout<XPU>	(pl, did, src, dst);
    case kLoss		: return new LayerLoss<XPU>	(pl, did, src, dst);
    case kNeuron	: return new LayerNeuron<XPU>	(pl, did, src, dst);
    case kPooling	: return new LayerPooling<XPU>	(pl, did, src, dst);
    default		: LOG (FATAL) << "not implemented layer type";
  }
  return NULL;
}
template LayerBase<GPU>* create_layer (ParaLayer &pl, const int did, TensorGPUf &src, TensorGPUf &dst);
template LayerBase<CPU>* create_layer (ParaLayer &pl, const int did, TensorCPUf &src, TensorCPUf &dst);

#endif
