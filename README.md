# nobodyDL——小人物深度学习
	nobodyDL的理念是简单、快速、有效
	简单——目前只有convolution、ReLU、max/avg pooling、dropout、loss这6种网络结构
	快速——不希望通过牺牲速度来提高精度
	有效——不希望通过牺牲精度来提高速度

	在这个版本中，我提供了一个在imagenet-1k分类任务上准确率大约为71.2%的模型（224*224 central crop）
	这是之前训练60轮的结果，最新的结果是训练30轮71.1%

	这个模型有很多优点，我认为按重要程度排序分别是：
	简单的网络结构，这使他能够无缝的代替VGGNet使用到目标检测和图像分割任务中，或者方便的移植进异构平台
	很高的分类精度，在开源软件中，目前是最高的
	很少的资源占用，在准确率超过70%的模型中，目前是最快的，也是显存占用最少的
	较少的模型参数，大约50兆二进制存储空间

	同时他也有一些缺点：
	并没有提供加速训练的代码
	并没有提供把模型文件转换为Caffe格式的工具
	并没有提供4卡并行的实现，目前我还没有相应的硬件

	剩下还有一些我没有想明白的问题:
	多尺度的训练和预测
	分布式算法
	Markdown语法
