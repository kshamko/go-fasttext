#include <iostream>
#include <istream>
#include <memory>
#include <streambuf>

#include "cbits.h"
#include "fasttext.h"
#include "real.h"

#include "args.cc"
#include "dictionary.cc"
#include "fasttext.cc"
#include "matrix.cc"
#include "model.cc"
#include "productquantizer.cc"
#include "quantmatrix.cc"
#include "utils.cc"
#include "vector.cc"

#include "json.hpp"

using json = nlohmann::json;

struct membuf : std::streambuf {
  membuf(char *begin, char *end) { this->setg(begin, begin, end); }
};

template <class Dest, class Source> inline Dest bit_cast(Source const &source) {
  static_assert(sizeof(Dest) == sizeof(Source),
                "size of destination and source objects must be equal");
  static_assert(std::is_trivially_copyable<Dest>::value,
                "destination type must be trivially copyable.");
  static_assert(std::is_trivially_copyable<Source>::value,
                "source type must be trivially copyable");

  Dest dest;
  std::memcpy(&dest, &source, sizeof(dest));
  return dest;
}

FastTextHandle NewHandle(const char *path) {
  auto model = new fasttext::FastText();
  model->loadModel(std::string(path));
  return bit_cast<FastTextHandle>(model);
}

void DeleteHandle(FastTextHandle handle) {
  auto model = bit_cast<fasttext::FastText *>(handle);
  if (model != nullptr) {
    delete model;
  }
}

char *Predict(FastTextHandle handle, char *query) {
  auto model = bit_cast<fasttext::FastText *>(handle);

  membuf sbuf(query, query + strlen(query));
  std::istream in(&sbuf);

  std::vector<std::pair<fasttext::real, std::string>> predictions;
  model->predict(in, 4, predictions);

  size_t ii = 0;
  auto res = json::array();
  for (const auto it : predictions) {
    float p = std::exp(it.first);
    res.push_back({
        {"index", ii++},
        {"probability", p},
        {"label", it.second},
    });
  }

  return strdup(res.dump().c_str());
}

char *Analogy(FastTextHandle handle, char *query) {
  auto model = bit_cast<fasttext::FastText *>(handle);

  model->analogies(10);

  size_t ii = 0;
  auto res = json::array();

  return strdup(res.dump().c_str());
}

char *Wordvec(FastTextHandle handle, char *query) {
  auto model = bit_cast<fasttext::FastText *>(handle);

  fasttext::Vector vec(model->getDimension());
  // fasttext::Matrix wordVectors(model->dict_->nwords(), model->getDimension());
  // model->precomputeWordVectors(wordVectors);
  model->getWordVector(vec, query);

  auto res = json::array();
  for (int i = 0; i < vec.size(); i++) {
    res.push_back(vec[i]);
  }

  return strdup(res.dump().c_str());
}

char *Sentencevec(FastTextHandle handle, char *query) {
  auto model = bit_cast<fasttext::FastText *>(handle);

  membuf sbuf(query, query + strlen(query));
  std::istream in(&sbuf);

  fasttext::Vector vec(model->getDimension());
  // fasttext::Matrix wordVectors(model->dict_->nwords(), model->getDimension());
  // model->precomputeWordVectors(wordVectors);
  model->getSentenceVector(in, vec);

  auto res = json::array();
  for (int i = 0; i < vec.size(); i++) {
    res.push_back(vec[i]);
  }

  return strdup(res.dump().c_str());
}
