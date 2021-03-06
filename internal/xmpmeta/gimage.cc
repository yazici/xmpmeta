// Copyright 2016 The XMPMeta Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xmpmeta/gimage.h"

#include <libxml/tree.h>

#include "glog/logging.h"
#include "xmpmeta/base64.h"
#include "xmpmeta/xmp_parser.h"
#include "xmpmeta/xml/const.h"
#include "xmpmeta/xml/deserializer_impl.h"
#include "xmpmeta/xml/utils.h"

using xmpmeta::xml::DeserializerImpl;
using xmpmeta::xml::GetFirstDescriptionElement;
using xmpmeta::xml::XmlConst;

namespace xmpmeta {
namespace {

const char kPrefix[] = "GImage";
const char kMime[] = "Mime";
const char kData[] = "Data";
const char kMimePng[] = "image/png";
const char kMimeJpeg[] = "image/jpeg";
const char kNamespaceHref[] = "http://ns.google.com/photos/1.0/image/";

}  // namespace

GImage::GImage() {}

void GImage::GetNamespaces(
    std::unordered_map<string, string>* ns_name_href_map) const {
  if (ns_name_href_map == nullptr) {
    LOG(ERROR) << "Namespace list or own namespace is null";
    return;
  }
  ns_name_href_map->emplace(kPrefix, kNamespaceHref);
}

const string& GImage::GetData() const { return data_; }

const string& GImage::GetMime() const { return mime_; }

std::unique_ptr<GImage> GImage::CreateFromData(const string& data,
                                               const string& mime) {
  if (data.empty() || mime.empty()) {
    return nullptr;
  }
  std::unique_ptr<GImage> gimage(new GImage());
  gimage->data_ = data;
  gimage->mime_ = mime;
  return gimage;
}

std::unique_ptr<GImage> GImage::FromXmp(const XmpData& xmp) {
  xml::DeserializerImpl
      std_deserializer(GetFirstDescriptionElement(xmp.StandardSection()));
  std::unique_ptr<GImage> gimage(new GImage());
  if (!std_deserializer.ParseString(kPrefix, kMime, &gimage->mime_)) {
    return nullptr;
  }
  xml::DeserializerImpl ext_deserializer(
      GetFirstDescriptionElement(xmp.ExtendedSection()));
  if (!ext_deserializer.ParseBase64(kPrefix, kData, &gimage->data_)) {
    return nullptr;
  }
  return gimage;
}

std::unique_ptr<GImage> GImage::FromJpegFile(const string& filename) {
  XmpData xmp;
  const bool kSkipExtended = false;
  if (!ReadXmpHeader(filename, kSkipExtended, &xmp)) {
    return nullptr;
  }
  return FromXmp(xmp);
}

bool GImage::IsPresent(const XmpData& xmp) {
  DeserializerImpl std_deserializer(
      GetFirstDescriptionElement(xmp.StandardSection()));
  string mime;
  return std_deserializer.ParseString(kPrefix, kMime, &mime);
}

bool GImage::IsPresent(const string& filename) {
  XmpData xmp;
  const bool kSkipExtended = true;
  if (!ReadXmpHeader(filename, kSkipExtended, &xmp)) {
    return false;
  }
  return IsPresent(xmp);
}

bool GImage::Serialize(xml::Serializer* std_serializer,
                       xml::Serializer* ext_serializer) const {
  if (std_serializer == nullptr || ext_serializer == nullptr) {
    LOG(ERROR) << "Serializer for standard section or extended section is null";
    return false;
  }

  string encoded;
  if (!EncodeBase64(data_, &encoded)) {
    LOG(WARNING) << "Data encoding failed";
    return false;
  }

  if (!std_serializer->WriteProperty(kPrefix, kMime, mime_)) {
    return false;
  }
  return ext_serializer->WriteProperty(kPrefix, kData, encoded);
}

}  // namespace xmpmeta
