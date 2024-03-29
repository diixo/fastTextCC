# fastTextСС
**fastTextСС** is extended version of [fastText](https://fasttext.cc/)

FastText is a library for efficient learning of word representations and sentence classification.

## Added features vs original **fastText**

* UTF-8 input file support
* Filtering by using stopwords-file (added **-stopwords** option), converting symbols to lower-case if stopwords (by default).
* Fix of handling special symbols (soft-hyphen, no-brake space, etc..)


## Table of contents

* [Usage](#usage)
* [Requirements](#requirements)
* [Building fastText](#building-fasttext)
   * [Building fastText using make (preferred)](#building-fasttext-using-make-preferred)
   * [Building and installing fastText using cmake](#building-and-installing-fasttext-using-cmake)
   * [Building and installing fastText for Python](#building-and-installing-fasttext-for-python)
* [Example use cases](#example-use-cases)
* [Loss functions](#loss-functions)
* [Unsupervised learning](#unsupervised-learning)
   * [Word representation learning](#word-representation-learning)
   * [CBOW](#cbow)
   * [Prediction](#prediction)
   * [Printing word vectors](#printing-word-vectors)
   * [Nearest neighbor queries](#nearest-neighbor-queries)
   * [Advanced reader: measure of similarity](#advanced-reader-measure-of-similarity)
   * [Word analogies](#word-analogies)
   * [Importance of character n-grams](#importance-of-character-n-grams)
   * [Obtaining word vectors for out-of-vocabulary words](#obtaining-word-vectors-for-out-of-vocabulary-words)
* [Supervised learning](#supervised-learning)
   * [Text classification](#text-classification)
* [Full documentation](#full-documentation)
* [References](#references)
   * [Enriching Word Vectors with Subword Information](#enriching-word-vectors-with-subword-information)
   * [Bag of Tricks for Efficient Text Classification](#bag-of-tricks-for-efficient-text-classification)
   * [FastText.zip: Compressing text classification models](#fasttextzip-compressing-text-classification-models)
* [License](#license)

## Usage
Running the binary without any argument will print the high level documentation, showing the different use cases supported by fastText:

```
>> ./fasttext
usage: fasttext <command> <args>

The commands supported by fasttext are:

  supervised              train a supervised classifier
  quantize                quantize a model to reduce the memory usage
  test                    evaluate a supervised classifier
  test-label              print labels with precision and recall scores
  predict                 predict most likely labels
  predict-prob            predict most likely labels with probabilities
  predict-next            predict most likely next word of words sequence with probabilities
  skipgram                train a skipgram model
  cbow                    train a cbow model
  print-word-vectors      print word vectors given a trained model
  print-sentence-vectors  print sentence vectors given a trained model
  print-ngrams            print ngrams given a trained model and word
  nn                      query for nearest neighbors
  analogies               query for analogies
  similarity              query similarity of word vs another word
  dump                    dump arguments, dictionary, input/output vectors
```
Current version was extended by two additional commands for CBOW-mode: **predict-next**, **similarity**.

## Requirements

Generally, **fastText** builds on modern Mac OS and Linux distributions.
Since it uses some C++11 features, it requires a compiler with good C++11 support.
These include :

* (g++-4.7.2 or newer) or (clang-3.3 or newer)

Compilation is carried out using a Makefile, so you will need to have a working **make**.
If you want to use **cmake** you need at least version 2.8.9.

For the word-similarity evaluation script you will need:

* Python 2.6 or newer
* NumPy & SciPy

For the python bindings (see the subdirectory python) you will need:

* Python version 2.7 or >=3.4
* NumPy & SciPy
* [pybind11](https://github.com/pybind/pybind11)

## Building fastText

We discuss building the latest stable version of fastText.

### Building fastText using make (preferred)

```
$ wget https://github.com/diixo/fastText/archive/v0.9.2.zip
$ unzip v0.9.2.zip
$ cd fastText-0.9.2
$ make
```

This will produce object files for all the classes as well as the main binary `fasttext`.
If you do not plan on using the default system-wide compiler, update the two macros defined at the beginning of the Makefile (CC and INCLUDES).

### Building and installing fastText using cmake

For now this is not part of a release, so you will need to clone the master branch.

```
$ git clone https://github.com/diixo/fastText.git
$ cd fastText
$ mkdir build && cd build && cmake ..
$ make && make install
```

This will create the fasttext binary and also all relevant libraries (shared, static, PIC).

### Building and installing fastText for Python

For now this is not part of a release, so you will need to clone the master branch.

```
$ git clone https://github.com/diixo/fastText.git
$ cd fastText
$ make
$ pip install .
```

For further information and introduction see python/README.md

## Example use cases

This library has two main use cases: word representation learning and text classification.
These were described in the two papers [1](#enriching-word-vectors-with-subword-information) and [2](#bag-of-tricks-for-efficient-text-classification).

## Loss-functions

* Negative-sampling loss = `-loss ns`, default for unsupervised.
* Softmax loss = `-loss softmax`, default for supervised.
* One-vs-all loss = `-loss ova`.
* Hierarchical softmax loss = `-loss hs`.

## Unsupervised learning
### Word representation learning

Word-representation modes **skipgram** and **cbow** use a default **-minCount** = 5, where **minCount** is minimal number of word occurences.

In order to learn word vectors, as described in [1](#enriching-word-vectors-with-subword-information), do:

```
$ ./fasttext skipgram -input data.txt -output model
```

where `data.txt` is a training file containing `UTF-8` encoded text.
By default the word vectors will take into account character n-grams from 3 to 6 characters.
At the end of optimization the program will save two files: `model.bin` and `model.vec`.
`model.vec` is a text file containing the word vectors, one per line.
`model.bin` is a binary file containing the parameters of the model along with the dictionary and all hyper parameters.
The binary file can be used later to compute word vectors or to restart the optimization.

### CBOW

The CBOW-model predicts the target word according to its context. The context is represented as a bag of the words contained in a fixed size window around the target word.

Let us illustrate this difference with an example: given the sentence 'Poets have been mysteriously silent on the subject of cheese' and the target word 'silent', a skipgram model tries to predict the target using a random close-by word, like 'subject' or 'mysteriously'. The cbow model takes all the words in a surrounding window, like {been, mysteriously, on, the}, and uses the sum of their vectors to predict the target.

```
$ ./fasttext cbow -input train-data.txt -output train-data -minCount 1 -stopwords stopwords.txt -epoch 100 -loss softmax -maxn 0
```
### Prediction

Predict next word for sequence words (only for CBOW-mode)

```
./fasttext predict-next model.bin
```
Now let's have a look on our predictions, we want as many prediction as possible (argument -1) and we want only labels with probability higher or equal to 0.5 :

```
./fasttext predict-next model.bin - -1 0.5
```
and then type the sentence:

*data structures*

The original sentence is: *data structures entity*. Now we get result :

```
entity
```
The argument `k` is optional, and is equal to `1` by default.
The argument `threshold` is optional, and is equal to `0` by default.
```
./fasttext predict-next model.bin text.txt
```
or:
```
./fasttext predict-next model.bin text.txt 10
```

The similarity param based on probability in value range: 0..1. Calculated by command: 

```
./fasttext similarity model.bin word1 word2
```

### Printing word vectors

Searching and printing word vectors directly from the `fil9.vec` file is cumbersome. Fortunately, there is a `print-word-vectors` functionality in fastText.

For example, we can print the word vectors of words asparagus, pidgey and yellow with the following command:

```
$ echo "asparagus pidgey yellow" | ./fasttext print-word-vectors result/fil9.bin
asparagus 0.46826 -0.20187 -0.29122 -0.17918 0.31289 -0.31679 0.17828 -0.04418 ...
pidgey -0.16065 -0.45867 0.10565 0.036952 -0.11482 0.030053 0.12115 0.39725 ...
yellow -0.39965 -0.41068 0.067086 -0.034611 0.15246 -0.12208 -0.040719 -0.30155 ...
```

By using Python:

```
>>> [model.get_word_vector(x) for x in ["asparagus", "pidgey", "yellow"]]
[array([-0.25751096, -0.18716481,  0.06921121,  0.06455903,  0.29168844,
        0.15426874, -0.33448914, -0.427215  ,  0.7813013 , -0.10600132,
        ...
        0.37090245,  0.39266172, -0.4555302 ,  0.27452755,  0.00467369],
      dtype=float32),
 array([-0.20613593, -0.25325796, -0.2422259 , -0.21067499,  0.32879013,
        0.7269511 ,  0.3782259 ,  0.11274897,  0.246764  , -0.6423613 ,
        ...
        0.46302193,  0.2530962 , -0.35795924,  0.5755718 ,  0.09843876],
      dtype=float32),
 array([-0.304823  ,  0.2543754 , -0.2198013 , -0.25421786,  0.11219151,
        0.38286993, -0.22636674, -0.54023844,  0.41095474, -0.3505803 ,
        ...
        0.54788435,  0.36740595, -0.5678512 ,  0.07523401, -0.08701935],
      dtype=float32)]
```

A nice feature is that you can also query for words that did not appear in your data! Indeed words are represented by the sum of its substrings. As long as the unknown word is made of known substrings, there is a representation of it!

As an example let's try with a misspelled word:

```
$ echo "enviroment" | ./fasttext print-word-vectors result/fil9.bin
```

You still get a word vector for it! But how good it is? Let's find out in the next sections!

### Nearest neighbor queries

A simple way to check the quality of a word vector is to look at its `nearest neighbors`. This give an intuition of the type of semantic information the vectors are able to capture.

This can be achieved with the `nearest neighbor` (nn) functionality. For example, we can query the 10 nearest neighbors of a word by running the following command:

```
$ ./fasttext nn result/fil9.bin
Pre-computing word vectors... done.
```
Then we are prompted to type our query word, let us try asparagus :

```
Query word? asparagus
beetroot 0.812384
tomato 0.806688
horseradish 0.805928
spinach 0.801483
licorice 0.791697
lingonberries 0.781507
asparagales 0.780756
lingonberry 0.778534
celery 0.774529
beets 0.773984
```

Nice! It seems that vegetable vectors are similar. Note that the nearest neighbor is the word asparagus itself, this means that this word appeared in the dataset. What about pokemons?

```
Query word? pidgey
pidgeot 0.891801
pidgeotto 0.885109
pidge 0.884739
pidgeon 0.787351
pok 0.781068
pikachu 0.758688
charizard 0.749403
squirtle 0.742582
beedrill 0.741579
charmeleon 0.733625
```

Different evolution of the same Pokemon have close-by vectors! But what about our misspelled word, is its vector close to anything reasonable? Let s find out:

```
Query word? enviroment
enviromental 0.907951
environ 0.87146
enviro 0.855381
environs 0.803349
environnement 0.772682
enviromission 0.761168
realclimate 0.716746
environment 0.702706
acclimatation 0.697196
ecotourism 0.697081
```

Thanks to the information contained within the word, the vector of our misspelled word matches to reasonable words! It is not perfect but the main information has been captured.

### Advanced reader: measure of similarity

In order to find nearest neighbors, we need to compute a similarity score between words. Our words are represented by continuous word vectors and we can thus apply simple similarities to them. In particular we use the cosine of the angles between two vectors. This similarity is computed for all words in the vocabulary, and the 10 most similar words are shown. Of course, if the word appears in the vocabulary, it will appear on top, with a similarity of 1.

### Word analogies

In a similar spirit, one can play around with word analogies. For example, we can see if our model can guess what is to France, and what Berlin is to Germany.

This can be done with the analogies functionality. It takes a word triplet (like Germany Berlin France) and outputs the analogy:

```
$ ./fasttext analogies result/fil9.bin
Pre-computing word vectors... done.
Query triplet (A - B + C)? berlin germany france
paris 0.896462
bourges 0.768954
louveciennes 0.765569
toulouse 0.761916
valenciennes 0.760251
montpellier 0.752747
strasbourg 0.744487
meudon 0.74143
bordeaux 0.740635
pigneaux 0.736122
```

The answer provided by our model is Paris, which is correct. Let's have a look at a less obvious example:

```
Query triplet (A - B + C)? psx sony nintendo
gamecube 0.803352
nintendogs 0.792646
playstation 0.77344
sega 0.772165
gameboy 0.767959
arcade 0.754774
playstationjapan 0.753473
gba 0.752909
dreamcast 0.74907
famicom 0.745298
```

Our model considers that the nintendo analogy of a psx is the gamecube, which seems reasonable. Of course the quality of the analogies depend on the dataset used to train the model and one can only hope to cover fields only in the dataset.

### Importance of character n-grams

Using subword-level information is particularly interesting to build vectors for unknown words. For example, the word gearshift does not exist on Wikipedia but we can still query its closest existing words:

```
Query word? gearshift
gearing 0.790762
flywheels 0.779804
flywheel 0.777859
gears 0.776133
driveshafts 0.756345
driveshaft 0.755679
daisywheel 0.749998
wheelsets 0.748578
epicycles 0.744268
gearboxes 0.73986
```

Most of the retrieved words share substantial substrings but a few are actually quite different, like cogwheel. You can try other words like sunbathe or grandnieces.

Now that we have seen the interest of subword information for unknown words, let's check how it compares to a model that does not use subword information. To train a model without subwords, just run the following command:

```
$ ./fasttext skipgram -input data/fil9 -output result/fil9-none -maxn 0
```

The results are saved in result/fil9-non.vec and result/fil9-non.bin.

To illustrate the difference, let us take an uncommon word in Wikipedia, like accomodation which is a misspelling of accommodation. Here is the nearest neighbors obtained without subwords:

```
$ ./fasttext nn result/fil9-none.bin
Query word? accomodation
sunnhordland 0.775057
accomodations 0.769206
administrational 0.753011
laponian 0.752274
ammenities 0.750805
dachas 0.75026
vuosaari 0.74172
hostelling 0.739995
greenbelts 0.733975
asserbo 0.732465
```

The result does not make much sense, most of these words are unrelated. On the other hand, using subword information gives the following list of nearest neighbors:

```
Query word? accomodation
accomodations 0.96342
accommodation 0.942124
accommodations 0.915427
accommodative 0.847751
accommodating 0.794353
accomodated 0.740381
amenities 0.729746
catering 0.725975
accomodate 0.703177
hospitality 0.701426
```

The nearest neighbors capture different variation around the word accommodation. We also get semantically related words such as amenities or catering.


### Obtaining word vectors for out-of-vocabulary words

The previously trained model can be used to compute word vectors for out-of-vocabulary words.
Provided you have a text file `queries.txt` containing words for which you want to compute vectors, use the following command:

```
$ ./fasttext print-word-vectors model.bin < queries.txt
```

This will output word vectors to the standard output, one vector per line.
This can also be used with pipes:

```
$ cat queries.txt | ./fasttext print-word-vectors model.bin
```

See the provided scripts for an example. For instance, running:

```
$ ./word-vector-example.sh
```

will compile the code, download data, compute word vectors and evaluate them on the rare words similarity dataset RW [Thang et al. 2013].

## Supervised learning
### Text classification

This library can also be used to train supervised text classifiers, for instance for sentiment analysis.
In order to train a text classifier using the method described in [2](#bag-of-tricks-for-efficient-text-classification), use:

```
$ ./fasttext supervised -input train.txt -output model
```

where `train.txt` is a text file containing a training sentence per line along with the labels.
By default, we assume that labels are words that are prefixed by the string `__label__`.
This will output two files: `model.bin` and `model.vec`.
Once the model was trained, you can evaluate it by computing the precision and recall at k (P@k and R@k) on a test set using:

```
$ ./fasttext test model.bin test.txt k
```

The argument `k` is optional, and is equal to `1` by default.

In order to obtain the k most likely labels for a piece of text, use:

```
$ ./fasttext predict model.bin test.txt k
```

or use `predict-prob` to also get the probability for each label

```
$ ./fasttext predict-prob model.bin test.txt k
```

where `test.txt` contains a piece of text to classify per line.
Doing so will print to the standard output the k most likely labels for each line.
The argument `k` is optional, and equal to `1` by default.
See `classification-example.sh` for an example use case.
In order to reproduce results from the paper [2](#bag-of-tricks-for-efficient-text-classification), run `classification-results.sh`, this will download all the datasets and reproduce the results from Table 1.

If you want to compute vector representations of sentences or paragraphs, please use `print-sentence-vectors`:

```
$ ./fasttext print-sentence-vectors model.bin < text.txt
```

This assumes that the `text.txt` file contains the paragraphs that you want to get vectors for.
The program will output one vector representation per line in the file.

You can also quantize a supervised model to reduce its memory usage with the following command:

```
$ ./fasttext quantize -output model
```
This will create a `.ftz` file with a smaller memory footprint. All the standard functionality, like `test` or `predict` work the same way on the quantized models:
```
$ ./fasttext test model.ftz test.txt
```
The quantization procedure follows the steps described in [3](#fasttextzip-compressing-text-classification-models). You can
run the script `quantization-example.sh` for an example.


## Full documentation

Invoke a command without arguments to list available arguments and their default values:

```
$ ./fasttext supervised
Empty input or output path.

The following arguments are mandatory:
  -input              training file path
  -output             output file path

The following arguments are optional:
  -verbose            verbosity level [2]

The following arguments for the dictionary are optional:
  -minCount           minimal number of word occurrences [1]
  -minCountLabel      minimal number of label occurrences [0]
  -wordNgrams         max length of word ngram [1]
  -bucket             number of buckets [2000000]
  -minn               min length of char ngram [0]
  -maxn               max length of char ngram [0]
  -t                  sampling threshold [0.0001]
  -label              labels prefix [__label__]

The following arguments for training are optional:
  -lr                 learning rate [0.1]
  -lrUpdateRate       change the rate of updates for the learning rate [100]
  -dim                size of word vectors [100]
  -ws                 size of the context window [5]
  -epoch              number of epochs [5]
  -neg                number of negatives sampled [5]
  -loss               loss function {ns, hs, softmax} [softmax]
  -thread             number of threads [1]
  -pretrainedVectors  pretrained word vectors for supervised learning []
  -saveOutput         whether output params should be saved [0]

The following arguments for quantization are optional:
  -cutoff             number of words and ngrams to retain [0]
  -retrain            finetune embeddings if a cutoff is applied [0]
  -qnorm              quantizing the norm separately [0]
  -qout               quantizing the classifier [0]
  -dsub               size of each sub-vector [2]
```

## References

Please cite [1](#enriching-word-vectors-with-subword-information) if using this code for learning word representations or [2](#bag-of-tricks-for-efficient-text-classification) if using for text classification.

### Enriching Word Vectors with Subword Information

[1] P. Bojanowski\*, E. Grave\*, A. Joulin, T. Mikolov, [*Enriching Word Vectors with Subword Information*](https://arxiv.org/abs/1607.04606)

```
@article{bojanowski2017enriching,
  title={Enriching Word Vectors with Subword Information},
  author={Bojanowski, Piotr and Grave, Edouard and Joulin, Armand and Mikolov, Tomas},
  journal={Transactions of the Association for Computational Linguistics},
  volume={5},
  year={2017},
  issn={2307-387X},
  pages={135--146}
}
```

### Bag of Tricks for Efficient Text Classification

[2] A. Joulin, E. Grave, P. Bojanowski, T. Mikolov, [*Bag of Tricks for Efficient Text Classification*](https://arxiv.org/abs/1607.01759)

```
@InProceedings{joulin2017bag,
  title={Bag of Tricks for Efficient Text Classification},
  author={Joulin, Armand and Grave, Edouard and Bojanowski, Piotr and Mikolov, Tomas},
  booktitle={Proceedings of the 15th Conference of the European Chapter of the Association for Computational Linguistics: Volume 2, Short Papers},
  month={April},
  year={2017},
  publisher={Association for Computational Linguistics},
  pages={427--431},
}
```

### FastText.zip: Compressing text classification models

[3] A. Joulin, E. Grave, P. Bojanowski, M. Douze, H. Jégou, T. Mikolov, [*FastText.zip: Compressing text classification models*](https://arxiv.org/abs/1612.03651)

```
@article{joulin2016fasttext,
  title={FastText.zip: Compressing text classification models},
  author={Joulin, Armand and Grave, Edouard and Bojanowski, Piotr and Douze, Matthijs and J{\'e}gou, H{\'e}rve and Mikolov, Tomas},
  journal={arXiv preprint arXiv:1612.03651},
  year={2016}
}
```

(\* These authors contributed equally.)


## License

fastText is MIT-licensed.
