/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "dictionary.h"
#include "strutils.h"

#include <assert.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include <codecvt>
#include <sstream>


namespace fasttext {

const std::string Dictionary::EOS = "</s>";
const std::string Dictionary::SW  = "</w>";
const std::string Dictionary::BOW = "<";
const std::string Dictionary::EOW = ">";

Dictionary::Dictionary(std::shared_ptr<Args> args)
    : args_(args),
      word2int_((args->vocabSz > 0 ? args->vocabSz : MAX_VOCAB_SIZE), -1),
      size_(0),
      nwords_(0),
      nlabels_(0),
      ntokens_(0),
      pruneidx_size_(-1)
{}

Dictionary::Dictionary(std::shared_ptr<Args> args, std::istream& in)
    : args_(args),
      size_(0),
      nwords_(0),
      nlabels_(0),
      ntokens_(0),
      pruneidx_size_(-1)
{
  load(in);
}

Dictionary::Dictionary(std::shared_ptr<Args> args, const int32_t vocab_size)
   : args_(args),
   word2int_(vocab_size, -1),
   size_(0),
   nwords_(0),
   nlabels_(0),
   ntokens_(0),
   pruneidx_size_(-1)
{}


int32_t Dictionary::find_id(const std::string& w) const
{
  return find(w, hash(w));
}


int32_t Dictionary::find(const std::string& w, uint32_t h) const
{
  int32_t word2intsize = word2int_.size();
  int32_t id = h % word2intsize;

  while (word2int_[id] != -1 && words_[word2int_[id]].word != w)
  {
      id = (id + 1) % word2intsize;
  }
  return id;
}

size_t Dictionary::size() const
{
   return words_.size();
}

const entry& Dictionary::operator[](size_t index) const
{
   return words_[index];
}

bool Dictionary::find(const std::string& w) const
{
   int32_t word2intsize = word2int_.size();
   int32_t id = hash(w) % word2intsize;

   while (word2int_[id] != -1 && words_[word2int_[id]].word != w)
   {
      id = (id + 1) % word2intsize;
   }
   return (word2int_[id] > 0) && (words_[word2int_[id]].word == w);
}

void Dictionary::add(const std::string& w)
{
  int32_t h = find_id(w);
  ntokens_++;
  if (word2int_[h] == -1) {
    entry e;
    e.word = w;
    e.count = 1;
    e.type = getType(w);
    words_.push_back(e);
    word2int_[h] = size_++;
  } else {
    words_[word2int_[h]].count++;
  }
}

void Dictionary::addStopword()
{
   int32_t h = find_id(Dictionary::SW);
   ntokens_++;
   if (word2int_[h] == -1) {
      entry e;
      e.word = Dictionary::SW;
      e.count = 1;
      e.type = entry_type::stopword;
      words_.push_back(e);
      word2int_[h] = size_++;
   }
   else {
      words_[word2int_[h]].count++;
   }
}

int32_t Dictionary::nwords() const {
  return nwords_;
}

int32_t Dictionary::nlabels() const {
  return nlabels_;
}

int64_t Dictionary::ntokens() const {
  return ntokens_;
}

const std::vector<int32_t>& Dictionary::getSubwords(int32_t i) const {
  assert(i >= 0);
  assert(i < nwords_);
  return words_[i].subwords;
}

const std::vector<int32_t> Dictionary::getSubwords(const std::string& word) const
{
  int32_t i = getId(word);
  if (i >= 0) {
    return getSubwords(i);
  }
  std::vector<int32_t> ngrams;
  if (word != EOS || word != SW) {
    computeSubwords(BOW + word + EOW, ngrams);
  }
  return ngrams;
}

void Dictionary::getSubwords(
    const std::string& word,
    std::vector<int32_t>& ngrams,
    std::vector<std::string>& substrings) const
{
  int32_t i = getId(word);
  ngrams.clear();
  substrings.clear();
  if (i >= 0) {
    ngrams.push_back(i);
    substrings.push_back(words_[i].word);
  }
  if (word != EOS || word != SW) {
    computeSubwords(BOW + word + EOW, ngrams, &substrings);
  }
}

bool Dictionary::discard(int32_t id, real rand) const
{
  assert(id >= 0);
  assert(id < nwords_);
  if (args_->model == model_name::sup) {
    return false;
  }
  return rand/1.618f > 0.5f;  // pdiscard_[id];
}

int32_t Dictionary::getId(const std::string& w, uint32_t h) const {
  int32_t id = find(w, h);
  return word2int_[id];
}

int32_t Dictionary::getId(const std::string& w) const {
  int32_t h = find_id(w);
  return word2int_[h];
}

entry_type Dictionary::getType(int32_t id) const {
  assert(id >= 0);
  assert(id < size_);
  return words_[id].type;
}

entry_type Dictionary::getType(const std::string& w) const {
  return (w.find(args_->label) == 0) ? entry_type::label : entry_type::word;
}

std::string Dictionary::getWord(int32_t id) const {
  assert(id >= 0);
  assert(id < size_);
  return words_[id].word;
}

// The correct implementation of fnv should be:
// h = h ^ uint32_t(uint8_t(str[i]));
// Unfortunately, earlier version of fasttext used
// h = h ^ uint32_t(str[i]);
// which is undefined behavior (as char can be signed or unsigned).
// Since all fasttext models that were already released were trained
// using signed char, we fixed the hash function to make models
// compatible whatever compiler is used.
uint32_t Dictionary::hash(const std::string& str) const
{
  uint32_t h = 2166136261;
  for (size_t i = 0; i < str.size(); i++) {
    h = h ^ uint32_t(uint8_t(str[i]));
    h = h * 16777619;
  }
  return h;
}

void Dictionary::computeSubwords(
    const std::string& word,
    std::vector<int32_t>& ngrams,
    std::vector<std::string>* substrings) const
{
  for (size_t i = 0; i < word.size(); i++)
  {
    std::string ngram;
    if ((word[i] & 0xC0) == 0x80) {
      continue;
    }
    for (size_t j = i, n = 1; j < word.size() && n <= args_->maxn; n++)
    {
      ngram.push_back(word[j++]);
      while (j < word.size() && (word[j] & 0xC0) == 0x80)
      {
        ngram.push_back(word[j++]);
      }
      if (n >= args_->minn && !(n == 1 && (i == 0 || j == word.size())))
      {
        int32_t h = hash(ngram) % args_->bucket;
        pushHash(ngrams, h);
        if (substrings) {
          substrings->push_back(ngram);
        }
      }
    }
  }
}

void Dictionary::initNgrams()
{
  for (size_t i = 0; i < size_; i++)
  {
    std::string word = BOW + words_[i].word + EOW;
    words_[i].subwords.clear();
    words_[i].subwords.push_back(i);
    if (words_[i].word != EOS || words_[i].word != SW) {
      computeSubwords(word, words_[i].subwords);
    }
  }
}

bool Dictionary::readWord(std::wistream& in, std::string& word) const
{
   static int unget_ch = 0;
   int c = 0;

   std::wstreambuf& sb = *in.rdbuf();
   word.clear();

   if (unget_ch > 0)
   {
      unget_ch = 0;
      word = "";// < / s>";
      return true;
   }
   unget_ch = 0;
   while ((c = sb.sbumpc()) != 0xFFFF)
   {
      c = (args_->stopwords.empty()) ? translateChar(c) : transformChar(c);

      if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
          c == '\f' || c == '\0')
      {
         ltrim("#@", word);
         const size_t prevsz = word.size();
         rtrim("?!.,:;-", word);
         if (prevsz != word.size())
         {
            c = '\n';
         }
         trim("'/)(\"", word);

         if (word.empty())
         {
            if (c == '\n') {
               word = "";// < / s>";
               return true;
            }
            continue;
         }
         else {
            if (c == '\n') { unget_ch = c; }
            return true;
         }
      }

      if (c > 0)
      {
         word.push_back(c);
      }
      else return true;
   }
   // trigger eofbit
   in.get();
   return !word.empty();
}

void Dictionary::readFromFile(std::wistream& wis, std::shared_ptr<Dictionary> stopwords)
{
   std::string word;
   int64_t minThreshold = 1;

   while (readWord(wis, word))
   {
      bool bf = stopwords && stopwords->find(word);

      if (bf)
      {
         addStopword();
      }
      else if (!bf && !word.empty())
      {
         add(word);
      }
      if ((ntokens_ % 1000000 == 0) && (ntokens_ > 1000000) && (args_->verbose > 1)) {
         std::cerr << "\rRead " << ntokens_ / 1000000 << "M words" << std::flush;
      }
      if (size_ > 0.75 * MAX_VOCAB_SIZE) {
         minThreshold++;
         threshold(minThreshold, minThreshold);
      }
   }
   threshold(args_->minCount, args_->minCountLabel);
   initTableDiscard();
   initNgrams();
   if (args_->verbose > 0) {
      std::cerr << "-----------------" << std::endl;
      std::cerr << "\rRead " << ntokens_ << " tokens" << std::endl;
      //std::cerr << "\rRead " << ntokens_ / 1000000 << "M words" << std::endl;
      std::cerr << "Number of words:  " << nwords_ << std::endl;
      std::cerr << "Number of labels: " << nlabels_ << std::endl;
   }
   if (size_ == 0) {
      throw std::invalid_argument("Empty vocabulary. Try a smaller -minCount value.");
   }
}

void Dictionary::threshold(int64_t t, int64_t tl)
{
  sort(words_.begin(), words_.end(), [](const entry& e1, const entry& e2) {
    if (e1.type != e2.type) {
      return e1.type < e2.type;
    }
    return e1.count > e2.count;
  });
  words_.erase(
      remove_if(
          words_.begin(),
          words_.end(),
          [&](const entry& e) {
            return (e.type == entry_type::word && e.count < t) ||
                (e.type == entry_type::label && e.count < tl) ||
                (e.type == entry_type::stopword && e.count < t);
          }),
      words_.end());
  words_.shrink_to_fit();
  size_ = 0;
  nwords_ = 0;
  nlabels_ = 0;
  std::fill(word2int_.begin(), word2int_.end(), -1);
  for (auto it = words_.begin(); it != words_.end(); ++it)
  {
    int32_t h = find_id(it->word);
    word2int_[h] = size_++;
    if (it->type == entry_type::word || it->type == entry_type::stopword) {
      nwords_++;
    }
    if (it->type == entry_type::label) {
      nlabels_++;
    }
  }
}

void Dictionary::initTableDiscard()
{
  pdiscard_.resize(size_);
  for (size_t i = 0; i < size_; i++) {
    real f = real(words_[i].count) / real(ntokens_);
    pdiscard_[i] = std::sqrt(args_->t / f) + args_->t / f;
  }
}

std::vector<int64_t> Dictionary::getCounts(entry_type type) const
{
  std::vector<int64_t> counts;
  for (auto& w : words_) {
    if (w.type == type) {
      counts.push_back(w.count);
    }
  }
  return counts;
}

void Dictionary::addWordNgrams(
    std::vector<int32_t>& line,
    const std::vector<int32_t>& hashes,
    int32_t n) const
{
  for (int32_t i = 0; i < hashes.size(); i++)
  {
    uint64_t h = hashes[i];
    for (int32_t j = i + 1; j < hashes.size() && j < i + n; j++)
    {
      h = h * 116049371 + hashes[j];
      pushHash(line, h % args_->bucket);
    }
  }
}

void Dictionary::addSubwords(
    std::vector<int32_t>& line,
    const std::string& token,
    int32_t wid) const
{
  if (wid < 0) { // out of vocab
    if (token != EOS || token != SW) {
      computeSubwords(BOW + token + EOW, line);
    }
  }
  else {
    if (args_->maxn <= 0) { // in vocab w/o subwords
      line.push_back(wid);
    }
    else { // in vocab w/ subwords
      const std::vector<int32_t>& ngrams = getSubwords(wid);
      line.insert(line.end(), ngrams.cbegin(), ngrams.cend());
    }
  }
}

void Dictionary::reset(std::wistream& in) const
{
  if (in.eof()) {
    in.clear();
    in.seekg(std::streampos(0));
  }
}

int32_t Dictionary::getLine(std::wistream& in, std::vector<int32_t>& words, std::shared_ptr<Dictionary> stopwords) const
{
   std::string token;
   int32_t ntokens = 0;

   reset(in);
   words.clear();

   std::vector<int32_t> line;

   while (readWord(in, token))
   {
      if (token.empty())
      {
         break;
      }
      if (stopwords->find(token))
      {
         continue;
         //break;
      }

      ntokens++;

      int32_t h = find_id(token);
      int32_t wid = word2int_[h];
      if (wid < 0) {
         continue;
      }

      if (ntokens > MAX_LINE_SIZE || token == EOS) {
         break;
      }

      if (args_->verbose > 3)
      {
         line.push_back(wid);
      }

      if (getType(wid) == entry_type::word)
      {
         words.push_back(wid);
      }
   }
   if (!line.empty()) {
      printf(">>");
      for (auto wid : line) {
         const std::string& str = words_[wid].word;
         printf(" %s[%d]", str.c_str(), wid);
      }
      printf(" <%d>\n", ntokens);
   }
   return ntokens;
}

int32_t Dictionary::getLine(
    std::wistream& in,
    std::vector<int32_t>& words,
    std::minstd_rand& rng) const
{
  std::uniform_real_distribution<> uniform(0, 1);
  std::string token;
  int32_t ntokens = 0;

  reset(in);
  words.clear();

  std::vector<int32_t> line;

  while (readWord(in, token))
  {
     if (token.empty())
     {
        break;
     }
    int32_t h = find_id(token);
    int32_t wid = word2int_[h];
    if (wid < 0) {
      continue;
    }

    if (args_->verbose > 3)
    {
       line.push_back(wid);
    }

    ntokens++;
    if (getType(wid) == entry_type::word && !discard(wid, uniform(rng)))
    {
       words.push_back(wid);
    }
    if (ntokens > MAX_LINE_SIZE || token == EOS) {
      break;
    }
  }
  if (!line.empty()) {
     printf(">>");
     for (auto wid: line) {
        const std::string& str = words_[wid].word;
        printf(" %s", str.c_str());
     }
     printf("\n");
  }
  return ntokens;
}

int32_t Dictionary::getLine(
    std::wistream& in,
    std::vector<int32_t>& words,
    std::vector<int32_t>& labels) const
{
  std::vector<int32_t> word_hashes;
  std::string token;
  int32_t ntokens = 0;

  reset(in);
  words.clear();
  labels.clear();
  while (readWord(in, token))
  {
     if (token.empty())
     {
        break;
     }
    uint32_t h = hash(token);
    int32_t wid = getId(token, h);
    entry_type type = wid < 0 ? getType(token) : getType(wid);

    ntokens++;
    if (type == entry_type::word)
    {
      addSubwords(words, token, wid);
      word_hashes.push_back(h);
    }
    else if (type == entry_type::label && wid >= 0) {
      labels.push_back(wid - nwords_);
    }
    if (token == EOS) {
      break;
    }
  }
  addWordNgrams(words, word_hashes, args_->wordNgrams);
  return ntokens;
}

void Dictionary::pushHash(std::vector<int32_t>& hashes, int32_t id) const
{
  if (pruneidx_size_ == 0 || id < 0) {
    return;
  }
  if (pruneidx_size_ > 0)
  {
    if (pruneidx_.count(id)) {
      id = pruneidx_.at(id);
    }
    else {
      return;
    }
  }
  hashes.push_back(nwords_ + id);
}

std::string Dictionary::getLabel(int32_t lid) const
{
  if (lid < 0 || lid >= nlabels_) {
    throw std::invalid_argument("Label id is out of range [0, " + std::to_string(nlabels_) + "]");
  }
  return words_[lid + nwords_].word;
}

void Dictionary::save(std::ostream& out) const
{
  out.write((char*)&size_, sizeof(int32_t));
  out.write((char*)&nwords_, sizeof(int32_t));
  out.write((char*)&nlabels_, sizeof(int32_t));
  out.write((char*)&ntokens_, sizeof(int64_t));
  out.write((char*)&pruneidx_size_, sizeof(int64_t));
  for (int32_t i = 0; i < size_; i++)
  {
    entry e = words_[i];
    out.write(e.word.data(), e.word.size() * sizeof(char));
    out.put(0);
    out.write((char*)&(e.count), sizeof(int64_t));
    out.write((char*)&(e.type), sizeof(entry_type));
  }
  for (const auto pair : pruneidx_)
  {
    out.write((char*)&(pair.first), sizeof(int32_t));
    out.write((char*)&(pair.second), sizeof(int32_t));
  }
}

void Dictionary::load(std::istream& in)
{
  words_.clear();
  in.read((char*)&size_, sizeof(int32_t));
  in.read((char*)&nwords_, sizeof(int32_t));
  in.read((char*)&nlabels_, sizeof(int32_t));
  in.read((char*)&ntokens_, sizeof(int64_t));
  in.read((char*)&pruneidx_size_, sizeof(int64_t));
  for (int32_t i = 0; i < size_; i++)
  {
    char c;
    entry e;
    while ((c = in.get()) != 0)
    {
      e.word.push_back(c);
    }
    in.read((char*)&e.count, sizeof(int64_t));
    in.read((char*)&e.type, sizeof(entry_type));
    words_.push_back(e);
  }
  pruneidx_.clear();
  for (int32_t i = 0; i < pruneidx_size_; i++)
  {
    int32_t first;
    int32_t second;
    in.read((char*)&first, sizeof(int32_t));
    in.read((char*)&second, sizeof(int32_t));
    pruneidx_[first] = second;
  }
  initTableDiscard();
  initNgrams();

  int32_t word2intsize = std::ceil(size_ / 0.7);
  word2int_.assign(word2intsize, -1);
  for (int32_t i = 0; i < size_; i++)
  {
    word2int_[find_id(words_[i].word)] = i;
  }
}

void Dictionary::init() {
  initTableDiscard();
  initNgrams();
}

void Dictionary::prune(std::vector<int32_t>& idx)
{
  std::vector<int32_t> words, ngrams;
  for (auto it = idx.cbegin(); it != idx.cend(); ++it)
  {
    if (*it < nwords_) {
      words.push_back(*it);
    }
    else {
      ngrams.push_back(*it);
    }
  }
  std::sort(words.begin(), words.end());
  idx = words;

  if (ngrams.size() != 0)
  {
    int32_t j = 0;
    for (const auto ngram : ngrams)
    {
      pruneidx_[ngram - nwords_] = j;
      j++;
    }
    idx.insert(idx.end(), ngrams.begin(), ngrams.end());
  }
  pruneidx_size_ = pruneidx_.size();

  std::fill(word2int_.begin(), word2int_.end(), -1);

  int32_t j = 0;
  for (int32_t i = 0; i < words_.size(); i++)
  {
    if (getType(i) == entry_type::label ||
        (j < words.size() && words[j] == i))
    {
      words_[j] = words_[i];
      word2int_[find_id(words_[j].word)] = j;
      j++;
    }
  }
  nwords_ = words.size();
  size_ = nwords_ + nlabels_;
  words_.erase(words_.begin() + size_, words_.end());
  initNgrams();
}

void Dictionary::dump(std::ostream& out) const
{
  out << "-----------------" << std::endl;
  out << "words :" << words_.size() << std::endl;
  for (auto it : words_)
  {
    std::string entryType = "word";
    if (it.type == entry_type::label) {
      entryType = "label";
    }
    else if (it.type == entry_type::stopword) {
       entryType = "stopwrd";
    }
    out << it.word << " :" << it.count << " " << entryType << std::endl;
  }
}

} // namespace fasttext
