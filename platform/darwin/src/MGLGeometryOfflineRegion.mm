#import "MGLGeometryOfflineRegion.h"

#if !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    #import <Cocoa/Cocoa.h>
#else
    #import <UIKit/UIKit.h>
#endif

#import "MGLOfflineRegion_Private.h"
#import "MGLGeometryOfflineRegion_Private.h"
#import "MGLFeature_Private.h"
#import "MGLShape_Private.h"
#import "MGLStyle.h"

@interface MGLGeometryOfflineRegion () <MGLOfflineRegion_Private, MGLGeometryOfflineRegion_Private>

@end

@implementation MGLGeometryOfflineRegion {
    NSURL *_styleURL;
}

@synthesize styleURL = _styleURL;

+ (BOOL)supportsSecureCoding {
    return YES;
}

- (instancetype)init {
    [NSException raise:@"Method unavailable"
                format:
     @"-[MGLGeometryOfflineRegion init] is unavailable. "
     @"Use -initWithStyleURL:bounds:fromZoomLevel:toZoomLevel: instead."];
    return nil;
}

- (instancetype)initWithStyleURL:(NSURL *)styleURL geometry:(MGLShape *)geometry fromZoomLevel:(double)minimumZoomLevel toZoomLevel:(double)maximumZoomLevel {
    if (self = [super init]) {
        if (!styleURL) {
            styleURL = [MGLStyle streetsStyleURLWithVersion:MGLStyleDefaultVersion];
        }

        if (!styleURL.scheme) {
            [NSException raise:@"Invalid style URL" format:
             @"%@ does not support setting a relative file URL as the style URL. "
             @"To download the online resources required by this style, "
             @"specify a URL to an online copy of this style. "
             @"For Mapbox-hosted styles, use the mapbox: scheme.",
             NSStringFromClass([self class])];
        }

        _styleURL = styleURL;
        _geometry = geometry;
        _minimumZoomLevel = minimumZoomLevel;
        _maximumZoomLevel = maximumZoomLevel;
    }
    return self;
}

- (instancetype)initWithOfflineRegionDefinition:(const mbgl::OfflineGeometryRegionDefinition &)definition {
    NSURL *styleURL = [NSURL URLWithString:@(definition.styleURL.c_str())];
    MGLShape *geometry = MGLShapeFromGeoJSON(definition.geometry);
    return [self initWithStyleURL:styleURL geometry:geometry fromZoomLevel:definition.minZoom toZoomLevel:definition.maxZoom];
}

- (const mbgl::OfflineRegionDefinition)offlineRegionDefinition {
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
    const float scaleFactor = [UIScreen instancesRespondToSelector:@selector(nativeScale)] ? [[UIScreen mainScreen] nativeScale] : [[UIScreen mainScreen] scale];
#elif TARGET_OS_MAC
    const float scaleFactor = [NSScreen mainScreen].backingScaleFactor;
#endif
    return mbgl::OfflineGeometryRegionDefinition(_styleURL.absoluteString.UTF8String,
                                                 _geometry.geometryObject,
                                                 _minimumZoomLevel, _maximumZoomLevel,
                                                 scaleFactor);
}

- (nullable instancetype)initWithCoder:(NSCoder *)coder {
    NSURL *styleURL = [coder decodeObjectForKey:@"styleURL"];
    MGLShape * geometry = [coder decodeObjectForKey:@"geometry"];
    double minimumZoomLevel = [coder decodeDoubleForKey:@"minimumZoomLevel"];
    double maximumZoomLevel = [coder decodeDoubleForKey:@"maximumZoomLevel"];

    return [self initWithStyleURL:styleURL geometry:geometry fromZoomLevel:minimumZoomLevel toZoomLevel:maximumZoomLevel];
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeObject:_styleURL forKey:@"styleURL"];
    [coder encodeObject:_geometry forKey:@"geometry"];
    [coder encodeDouble:_maximumZoomLevel forKey:@"maximumZoomLevel"];
    [coder encodeDouble:_minimumZoomLevel forKey:@"minimumZoomLevel"];
}

- (id)copyWithZone:(nullable NSZone *)zone {
    return [[[self class] allocWithZone:zone] initWithStyleURL:_styleURL geometry:_geometry fromZoomLevel:_minimumZoomLevel toZoomLevel:_maximumZoomLevel];
}

- (BOOL)isEqual:(id)other {
    if (other == self) {
        return YES;
    }
    if (![other isKindOfClass:[self class]]) {
        return NO;
    }

    MGLGeometryOfflineRegion *otherRegion = other;
    return (_minimumZoomLevel == otherRegion->_minimumZoomLevel
            && _maximumZoomLevel == otherRegion->_maximumZoomLevel
            && _geometry.geometryObject == otherRegion->_geometry.geometryObject
            && [_styleURL isEqual:otherRegion->_styleURL]);
}

- (NSUInteger)hash {
    return (_styleURL.hash
            + _geometry.hash
            + @(_minimumZoomLevel).hash + @(_maximumZoomLevel).hash);
}

@end
