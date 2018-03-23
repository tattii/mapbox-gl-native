#include <mbgl/storage/offline.hpp>
#include <mbgl/util/tile_cover.hpp>
#include <mbgl/util/tileset.hpp>
#include <mbgl/util/projection.hpp>

#include <mapbox/geojson.hpp>
#include <mapbox/geojson/rapidjson.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cmath>

namespace mbgl {

// Generic functions

template <class RegionDefinition>
Range<uint8_t> coveringZoomRange(const RegionDefinition& definition, style::SourceType type, uint16_t tileSize, const Range<uint8_t>& zoomRange) {
    double minZ = std::max<double>(util::coveringZoomLevel(definition.minZoom, type, tileSize), zoomRange.min);
    double maxZ = std::min<double>(util::coveringZoomLevel(definition.maxZoom, type, tileSize), zoomRange.max);

    assert(minZ >= 0);
    assert(maxZ >= 0);
    assert(minZ < std::numeric_limits<uint8_t>::max());
    assert(maxZ < std::numeric_limits<uint8_t>::max());
    return { static_cast<uint8_t>(minZ), static_cast<uint8_t>(maxZ) };
}

template <class RegionDefinition, class RegionGeometry>
std::vector<CanonicalTileID> _tileCover(const RegionDefinition& definition, const RegionGeometry& geometry,
                                        style::SourceType type, uint16_t tileSize, const Range<uint8_t>& zoomRange) {
    const Range<uint8_t> clampedZoomRange = coveringZoomRange(definition, type, tileSize, zoomRange);

    std::vector<CanonicalTileID> result;

    for (uint8_t z = clampedZoomRange.min; z <= clampedZoomRange.max; z++) {
        for (const auto& tile : util::tileCover(geometry, z)) {
            result.emplace_back(tile.canonical);
        }
    }

    return result;
}

template <class RegionDefinition, class RegionGeometry>
uint64_t _tileCount(const RegionDefinition& definition, const RegionGeometry& geometry,
                    style::SourceType type, uint16_t tileSize, const Range<uint8_t>& zoomRange) {

    const Range<uint8_t> clampedZoomRange = coveringZoomRange(definition, type, tileSize, zoomRange);
    unsigned long result = 0;;
    for (uint8_t z = clampedZoomRange.min; z <= clampedZoomRange.max; z++) {
        result +=  util::tileCount(geometry, z);
    }

    return result;
}


// OfflineTilePyramidRegionDefinition

OfflineTilePyramidRegionDefinition::OfflineTilePyramidRegionDefinition(
    std::string styleURL_, LatLngBounds bounds_, double minZoom_, double maxZoom_, float pixelRatio_)
    : styleURL(std::move(styleURL_)),
      bounds(std::move(bounds_)),
      minZoom(minZoom_),
      maxZoom(maxZoom_),
      pixelRatio(pixelRatio_) {
    if (minZoom < 0 || maxZoom < 0 || maxZoom < minZoom || pixelRatio < 0 ||
        !std::isfinite(minZoom) || std::isnan(maxZoom) || !std::isfinite(pixelRatio)) {
        throw std::invalid_argument("Invalid offline region definition");
    }
}

std::vector<CanonicalTileID> OfflineTilePyramidRegionDefinition::tileCover(style::SourceType type, uint16_t tileSize, const Range<uint8_t>& zoomRange) const {
    return _tileCover(*this, bounds, type, tileSize, zoomRange);
}

uint64_t OfflineTilePyramidRegionDefinition::tileCount(style::SourceType type, uint16_t tileSize, const Range<uint8_t>& zoomRange) const {
    return _tileCount(*this, bounds, type, tileSize, zoomRange);
}


// OfflineGeometryRegionDefinition

OfflineGeometryRegionDefinition::OfflineGeometryRegionDefinition(std::string styleURL_, Geometry<double> geometry_, double minZoom_, double maxZoom_, float pixelRatio_)
    : styleURL(styleURL_)
    , geometry(std::move(geometry_))
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , pixelRatio(pixelRatio_) {
    if (minZoom < 0 || maxZoom < 0 || maxZoom < minZoom || pixelRatio < 0 ||
        !std::isfinite(minZoom) || std::isnan(maxZoom) || !std::isfinite(pixelRatio)) {
        throw std::invalid_argument("Invalid offline region definition");
    }
}

std::vector<CanonicalTileID> OfflineGeometryRegionDefinition::tileCover(style::SourceType type, uint16_t tileSize,
                                                                        const Range<uint8_t> &zoomRange) const {
    return _tileCover(*this, geometry, type, tileSize, zoomRange);
}

uint64_t OfflineGeometryRegionDefinition::tileCount(style::SourceType type, uint16_t tileSize,
                                                    const Range<uint8_t> &zoomRange) const {
    return _tileCount(*this, geometry, type, tileSize, zoomRange);
}

OfflineRegionDefinition decodeOfflineRegionDefinition(const std::string& region) {
    rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator> doc;
    doc.Parse<0>(region.c_str());

    // validation

    auto hasValidBounds = [&] {
        return doc.HasMember("bounds") && doc["bounds"].IsArray() && doc["bounds"].Size() == 4
               && doc["bounds"][0].IsDouble() && doc["bounds"][1].IsDouble()
               && doc["bounds"][2].IsDouble() && doc["bounds"][3].IsDouble();
    };

    auto hasValidGeometry = [&] {
        return doc.HasMember("geometry") && doc["geometry"].IsObject();
    };

    if (doc.HasParseError()
        || !doc.HasMember("style_url") || !doc["style_url"].IsString()
        || !(hasValidBounds() || hasValidGeometry())
        || !doc.HasMember("min_zoom") || !doc["min_zoom"].IsDouble()
        || (doc.HasMember("max_zoom") && !doc["max_zoom"].IsDouble())
        || !doc.HasMember("pixel_ratio") || !doc["pixel_ratio"].IsDouble()) {
        throw std::runtime_error("Malformed offline region definition");
    }

    // Common properties

    std::string styleURL { doc["style_url"].GetString(), doc["style_url"].GetStringLength() };
    double minZoom = doc["min_zoom"].GetDouble();
    double maxZoom = doc.HasMember("max_zoom") ? doc["max_zoom"].GetDouble() : INFINITY;
    float pixelRatio = doc["pixel_ratio"].GetDouble();
    
    if (doc.HasMember("bounds")) {
        return OfflineTilePyramidRegionDefinition{
                styleURL,
                LatLngBounds::hull(
                         LatLng(doc["bounds"][0].GetDouble(), doc["bounds"][1].GetDouble()),
                         LatLng(doc["bounds"][2].GetDouble(), doc["bounds"][3].GetDouble())),
                minZoom, maxZoom, pixelRatio };
    } else {
        return OfflineGeometryRegionDefinition{
                styleURL,
                mapbox::geojson::convert<Geometry<double>>(doc["geometry"].GetObject()),
                minZoom, maxZoom, pixelRatio };
    };

}

std::string encodeOfflineRegionDefinition(const OfflineRegionDefinition& region) {
    rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator> doc;
    doc.SetObject();

    // Encode common properties
    region.match([&](auto& _region) {
        doc.AddMember("style_url", rapidjson::StringRef(_region.styleURL.data(), _region.styleURL.length()), doc.GetAllocator());
        doc.AddMember("min_zoom", _region.minZoom, doc.GetAllocator());
        if (std::isfinite(_region.maxZoom)) {
            doc.AddMember("max_zoom", _region.maxZoom, doc.GetAllocator());
        }

        doc.AddMember("pixel_ratio", _region.pixelRatio, doc.GetAllocator());
    });

    // Encode specific properties
    region.match(
            [&] (const OfflineTilePyramidRegionDefinition& _region) {
                rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::CrtAllocator> bounds(rapidjson::kArrayType);
                bounds.PushBack(_region.bounds.south(), doc.GetAllocator());
                bounds.PushBack(_region.bounds.west(), doc.GetAllocator());
                bounds.PushBack(_region.bounds.north(), doc.GetAllocator());
                bounds.PushBack(_region.bounds.east(), doc.GetAllocator());
                doc.AddMember("bounds", bounds, doc.GetAllocator());

            },
            [&] (const OfflineGeometryRegionDefinition& _region) {
                doc.AddMember("geometry", mapbox::geojson::convert(_region.geometry, doc.GetAllocator()), doc.GetAllocator());

            }
    );

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}


// OfflineRegion

OfflineRegion::OfflineRegion(int64_t id_,
                             OfflineRegionDefinition definition_,
                             OfflineRegionMetadata metadata_)
    : id(id_),
      definition(std::move(definition_)),
      metadata(std::move(metadata_)) {
}

OfflineRegion::OfflineRegion(OfflineRegion&&) = default;
OfflineRegion::~OfflineRegion() = default;

const OfflineRegionDefinition& OfflineRegion::getDefinition() const {
    return definition;
}

const OfflineRegionMetadata& OfflineRegion::getMetadata() const {
    return metadata;
}

int64_t OfflineRegion::getID() const {
    return id;
}
} // namespace mbgl
