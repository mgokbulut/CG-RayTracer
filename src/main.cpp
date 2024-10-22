#include "bounding_volume_hierarchy.h"
#include "disable_all_warnings.h"
#include "draw.h"
#include "image.h"
#include "ray_tracing.h"
#include "screen.h"
#include "trackball.h"
#include "window.h"
// Disable compiler warnings in third-party code (which we cannot change).
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
DISABLE_WARNINGS_POP()
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <type_traits>
#ifdef USE_OPENMP
#include <omp.h>
#endif

// This is the main application. The code in here does not need to be modified.
constexpr glm::ivec2 windowResolution{800, 800};
const std::filesystem::path dataPath{DATA_DIR};
const std::filesystem::path outputPath{OUTPUT_DIR};

bool bloom = false;
bool blur = false;
bool antiAliasing = false;

enum class ViewMode
{
    Rasterization = 0,
    RayTracing = 1
};

/**
@author Alex
*/
static glm::vec3 randomUnitVector()
{
    std::random_device mch;
    std::default_random_engine generator(mch());
    std::normal_distribution<float> distribution(0.0f, 1.0f);
    float y = distribution(generator);
    float x = distribution(generator);
    float s = distribution(generator);
    /*float y = rand() - RAND_MAX / 2;
    float x = rand() - RAND_MAX / 2;
    float s = rand() - RAND_MAX / 2;*/
    
    return glm::normalize(glm::vec3(y, x, s));
}

static glm::vec3 specularOneLight(Ray &ray, const PointLight &light, const glm::vec3 &fromPosToLight, HitInfo &hitInfo)
{
    glm::vec3 fromCamToPos = ray.direction;
    glm::vec3 reflected = glm::normalize(glm::reflect(fromCamToPos, hitInfo.normal));

    // draw the normal
    //drawRay(Ray{ ray.origin + ray.direction * ray.t, hitInfo.normal, glm::length(fromCamToPos) }, glm::vec3{ 0.0f, 0.0f, 1.0f });

    float specularCos = glm::dot(reflected, fromPosToLight);
    if (specularCos <= 0)
    {
        // the reflection is not counted because the angle is too high
        //drawRay(Ray{ ray.origin + ray.direction * ray.t, reflected, glm::length(fromCamToPos) }, glm::vec3{ 1.0f, 0.0f, 0.0f });
        return glm::vec3(0);
    }

    // Is * Ks * cos(theta)
    glm::vec3 result = light.color * hitInfo.material.ks * pow(specularCos, hitInfo.material.shininess);
    // draw the reflection with the specular colour
    //drawRay(Ray{ ray.origin + ray.direction * ray.t, reflected, glm::length(fromCamToPos) }, result);
    return result;
}

static glm::vec3 diffuseOneLight(Ray &ray, const PointLight &light, const glm::vec3 &fromPosToLight, HitInfo &hitInfo)
{
    drawRay(Ray{ray.origin + ray.direction * ray.t, hitInfo.normal, 5.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
    float diffuseCos = glm::dot(fromPosToLight, hitInfo.normal);

    if (diffuseCos <= 0)
    { // this point is facing away from the light
        drawRay(Ray{ray.origin + ray.direction * ray.t, fromPosToLight, 5.0f}, glm::vec3{1.0f, 0.0f, 0.0f});
        return glm::vec3(0);
    }

    drawRay(Ray{ray.origin + ray.direction * ray.t, fromPosToLight, 5.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
    // Id * Kd * cos(theta)
    return light.color * hitInfo.material.kd * diffuseCos;
}
/**
* @author Alex, added bvh to the function signature
* first for loop
**/

static bool pointInShadow(glm::vec3 &pointOn, const PointLight &light, const BoundingVolumeHierarchy &bvh)
{
    glm::vec3 fromPosToLight = light.position - pointOn;
    Ray ray{pointOn, glm::normalize(fromPosToLight), std::numeric_limits<float>::max()};

    // set an offset to the ray not to always intersect the object at which we have our point
    float epsilon = 0.001;
    ray.origin += epsilon * ray.direction;

    // only check the distance of the intersected object if we intersected something
    HitInfo shadowRayHitInfo;
    if (bvh.intersect(ray, shadowRayHitInfo))
    {
        //drawRay(ray, glm::vec3(0.0f, 1.0f, 0.0f));
        // if there is an object between us and the light source, we are in shadow
        if (ray.t + epsilon >= glm::length(fromPosToLight))
        {
            //drawRay(ray, glm::vec3(0.0f, 1.0f, 0.0f));
            return false;
        }

        Ray afterHit;
        afterHit.origin = ray.origin + ray.t * ray.direction;
        afterHit.direction = ray.direction;
        afterHit.t = glm::length(fromPosToLight) - ray.t;
        //drawRay(afterHit, glm::vec3(1.0f, 0.0f, 0.0f));
        return true;
    }

    //drawRay(ray, glm::vec3(0.0f, 0.0f, 1.0f));
    return false;
}

// static glm::vec3 shading(Ray &ray, HitInfo &hitInfo, const Scene &scene, const BoundingVolumeHierarchy &bvh)
// {
//     const std::vector<PointLight> &pointLights = scene.pointLights;
//     glm::vec3 pointOn = ray.origin + ray.direction * ray.t;
//     glm::vec3 result(0.0f);

//     for (const PointLight &light : pointLights)
//     {
//         const glm::vec3 fromPosToLight = glm::normalize(light.position - pointOn);
//         if (pointInShadow(pointOn, light, bvh))
//         {
//             continue;
//         }

//         glm::vec3 diffuse = diffuseOneLight(ray, light, fromPosToLight, hitInfo);
//         glm::vec3 specular = specularOneLight(ray, light, fromPosToLight, hitInfo);
//         result += diffuse;
//         result += specular;
//     }

//     return result;
// }

static glm::vec3 shading(Ray &ray, HitInfo &hitInfo, const Scene &scene, const BoundingVolumeHierarchy &bvh)
{
    const std::vector<PointLight> &pointLights = scene.pointLights;
    const std::vector<SphericalLight> &sphericalLights = scene.sphericalLight;
    glm::vec3 pointOn = ray.origin + ray.direction * ray.t;
    glm::vec3 result(0.0f);
    float softShadowCounter = 0.0f;

    for (const SphericalLight &spherical : sphericalLights)
    {
        const PointLight &light = {spherical.position, spherical.color};

        const glm::vec3 fromPosToLight = glm::normalize(light.position - pointOn);
        glm::vec3 diffuse = diffuseOneLight(ray, light, fromPosToLight, hitInfo);
        glm::vec3 specular = specularOneLight(ray, light, fromPosToLight, hitInfo);
        softShadowCounter = 0.0f;
        for (int i = 1; i <= 200; i++)
        {
            glm::vec3 randomPointOnSphere = spherical.position + spherical.radius * randomUnitVector();
            Ray newRay = {pointOn + (float)(0.001) * (glm::normalize(randomPointOnSphere - pointOn)), glm::normalize(randomPointOnSphere - pointOn), length(newRay.origin - randomPointOnSphere)};
            HitInfo newHitInfo;
            float lightT = length(newRay.origin - randomPointOnSphere);
            if (!(bvh.intersect(newRay, newHitInfo)))
            {
                softShadowCounter += 1.0f;
                drawRay(newRay, glm::vec3(1));
            }
            else
            {
                if (newRay.t > lightT)
                {
                    softShadowCounter += 1.0f;
                    drawRay(newRay, glm::vec3(1));
                }
                else
                {
                    drawRay(newRay, glm::vec3(1, 0, 0));
                }
            }
        }
        softShadowCounter = softShadowCounter / 200.0f;

        //const PointLight &light = {spherical.position, spherical.color};

        //const glm::vec3 fromPosToLight = glm::normalize(light.position - pointOn);
        /**if (pointInShadow(pointOn, light, bvh))
        {
            continue;
        }*/

        //glm::vec3 diffuse = diffuseOneLight(ray, light, fromPosToLight, hitInfo);
        //glm::vec3 specular = specularOneLight(ray, light, fromPosToLight, hitInfo);
        result += diffuse * softShadowCounter;
        /**std::cout<<"Result is:"<<"x:"<<result.x <<" "<<"y:"<<result.y << " " <<"z:"<<result.z << std::endl;
        std::cout <<"Diffuse is:"<<"x:"<< diffuse.x << " " <<"y:"<< diffuse.y << " " <<"z:"<< diffuse.z << std::endl;
        std::cout <<"Specular is:"<<"x"<< specular.x << " " <<"y:"<< specular.y << " " <<"z:"<< specular.z << std::endl;
        std::cout << std::endl;*/
        result += specular * softShadowCounter;
    }

    for (const PointLight &light : pointLights)
    {
        const glm::vec3 fromPosToLight = glm::normalize(light.position - pointOn);
        if (pointInShadow(pointOn, light, bvh))
        {
            continue;
        }

        glm::vec3 diffuse = diffuseOneLight(ray, light, fromPosToLight, hitInfo);
        glm::vec3 specular = specularOneLight(ray, light, fromPosToLight, hitInfo);
        result += diffuse;
        result += specular;
    }

    return result;
}

// Recursive Ray tracing methods
static void trace(int level, Ray ray, glm::vec3 &color, const Scene &scene, const BoundingVolumeHierarchy &bvh);
static void shade(int level, Ray ray, glm::vec3 &color, const Scene &scene, const BoundingVolumeHierarchy &bvh, HitInfo &hitInfo);

static void shade(int level, Ray ray, glm::vec3 &color, const Scene &scene, const BoundingVolumeHierarchy &bvh, HitInfo &hitInfo)
{
    //ComputeDirectLight
    glm::vec3 directColor = shading(ray, hitInfo, scene, bvh);

    if (hitInfo.material.ks.x <= 0.01f, hitInfo.material.ks.y <= 0.01f, hitInfo.material.ks.z <= 0.01f)
    {
        color = directColor;
        return;
    }
    //ComputeReflectedRay
    glm::vec3 fromCamToPos = ray.direction;
    glm::vec3 reflected = glm::normalize(glm::reflect(fromCamToPos, hitInfo.normal));
    Ray reflectedRay = {ray.origin + ray.direction * ray.t, reflected, glm::length(fromCamToPos)};
    float epsilon = 0.001;
    reflectedRay.origin += epsilon * reflectedRay.direction;
    //drawRay(reflectedRay, glm::vec3{1.0f, 0.0f, 0.0f});

    glm::vec3 reflectedColor;
    trace(level + 1, reflectedRay, reflectedColor, scene, bvh);
    // std::cout << hitInfo.material.ks.x << std::endl;

    color = directColor + reflectedColor * hitInfo.material.ks;
}
static void trace(int level, Ray ray, glm::vec3 &color, const Scene &scene, const BoundingVolumeHierarchy &bvh)
{
    if (level >= 2)
    {
        //std::cout << "end" << std::endl;
        color = glm::vec3(0.0f);
        return;
    }
    //std::cout << level << std::endl;

    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo))
    {
        // Draw a white debug ray.
        drawRay(ray, glm::vec3(1.0f));

        //std::cout << ray.origin.x << " " << ray.origin.y << " " << ray.origin.z << " " << ray.direction.x << " " << ray.direction.y << " " << ray.direction.z << std::endl;

        // Get the resulting shading
        glm::vec3 shadingResult = shading(ray, hitInfo, scene, bvh);

        shade(level, ray, color, scene, bvh, hitInfo);
    }
    else
    {
        // Draw a red debug ray if the ray missed.
        drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
        // Set the color of the pixel to black if the ray misses.
        color = glm::vec3(0.0f);
    }
}

// NOTE(Mathijs): separate function to make recursion easier (could also be done with lambda + std::function).
static glm::vec3 getFinalColor(const Scene &scene, const BoundingVolumeHierarchy &bvh, Ray ray)
{
    //std::cout << "called" << std::endl;
    glm::vec3 color;

    //Bug Example
    //ray.origin = {0.0268146, 0.313131, 0.523811};
    //ray.direction = {0.2711, 0.416066, -0.867983};

    trace(0, ray, color, scene, bvh);

    return color;
}
// static glm::vec3 getFinalColor(const Scene &scene, const BoundingVolumeHierarchy &bvh, Ray ray)
// {
//     HitInfo hitInfo;
//     if (bvh.intersect(ray, hitInfo))
//     {
//         // Draw a white debug ray.
//         drawRay(ray, glm::vec3(1.0f));
static void blurEffect(const Scene &scene, const Trackball &camera, const BoundingVolumeHierarchy &bvh, Screen &screen, std::vector<glm::vec3> &matrixPixels)
{

    // Trackball cameraNew = camera;
    // float var_ = 0.01f;
    // for (size_t i = 0; i < 15; i++)
    // {
    //     cameraNew.setLookAt(glm::vec3(var_, 0, 0));
    //     for (int y = 0; y < windowResolution.y; y++)
    //     {
    //         for (int x = 0; x != windowResolution.x; x++)
    //         {
    //             // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
    //             const glm::vec2 normalizedPixelPos{
    //                 float(x) / windowResolution.x * 2.0f - 1.0f,
    //                 float(y) / windowResolution.y * 2.0f - 1.0f};
    //             const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
    //             glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

    //             matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
    //         }
    //     }
    //     var_ += 0.01f;
    // }

    Trackball cameraNew = camera;
    cameraNew.setLookAt(glm::vec3(0.01, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.02, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.03, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.04, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.05, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.06, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.07, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.08, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.09, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.10, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.11, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.12, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.13, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.14, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
        }
    }

    cameraNew.setLookAt(glm::vec3(0.15, 0, 0));
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = cameraNew.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);

            matrixPixels.at(y * windowResolution.x + x) += getFinalColor(scene, bvh, cameraRay);
            screen.setPixel(x, y, glm::vec3(matrixPixels.at(y * windowResolution.x + x).x / 16, matrixPixels.at(y * windowResolution.x + x).y / 16, matrixPixels.at(y * windowResolution.x + x).z / 16));
        }
    }
}

static void bloomEffect(std::vector<glm::vec3> &matrixPixels, std::vector<glm::vec3> &matrixColorsScreen, Screen &screen, const Scene &scene, const Trackball &camera, const BoundingVolumeHierarchy &bvh)
{
    int counter = 1;
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            counter = 1;
            for (int i = -10; i < 11; i++)
            {
                if (y + i < 0 || y + i > windowResolution.y - 1)
                    continue;
                else
                {
                    for (int j = -10; j < 11; j++)
                    {
                        if (i == 0 && j == 0)
                            continue;
                        if (x + j < 0 || x + j > windowResolution.x - 1)
                            continue;
                        else
                        {
                            matrixColorsScreen.at((y * windowResolution.x) + x) += matrixColorsScreen.at(((y + i) * windowResolution.x) + (x + j));
                            counter++;
                        }
                    }
                }
            }
            matrixColorsScreen.at((y * windowResolution.x) + x) = glm::vec3(matrixColorsScreen.at((y * windowResolution.x) + x).x / counter, matrixColorsScreen.at((y * windowResolution.x) + x).y / counter, matrixColorsScreen.at((y * windowResolution.x) + x).z / counter);

            const glm::vec2 normalizedPixelPos{
                float(x) / windowResolution.x * 2.0f - 1.0f,
                float(y) / windowResolution.y * 2.0f - 1.0f};
            const Ray cameraRay = camera.generateRay(normalizedPixelPos);
            glm::vec3 color = getFinalColor(scene, bvh, cameraRay);
            if (bloom == true)
            {
                screen.setPixel(x, y, matrixColorsScreen.at((y * windowResolution.x) + x) + color);
                matrixPixels.at(y * windowResolution.x + x) += matrixColorsScreen.at((y * windowResolution.x) + x) + color;
            }
        }
    }
}
//         // Get the resulting shading
//         glm::vec3 shadingResult = shading(ray, hitInfo, scene);

//         // Set the color of the pixel to white if the ray hits.
//         return shadingResult;
//     }
//     else
//     {
//         // Draw a red debug ray if the ray missed.
//         drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f));
//         // Set the color of the pixel to black if the ray misses.
//         return glm::vec3(0.0f);
//     }
// }

static void setOpenGLMatrices(const Trackball &camera);
static void renderOpenGL(const Scene &scene, const Trackball &camera, int selectedLight);

// This is the main rendering function. You are free to change this function in any way (including the function signature).
static void renderRayTracing(const Scene &scene, const Trackball &camera, const BoundingVolumeHierarchy &bvh, Screen &screen)
{
    std::vector<glm::vec3> matrixColorsScreen(windowResolution.x * windowResolution.y + 1);
    std::vector<glm::vec3> matrixPixels(windowResolution.x * windowResolution.y + 1);

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for (int y = 0; y < windowResolution.y; y++)
    {
        for (int x = 0; x != windowResolution.x; x++)
        {
            glm::vec3 color;
            Ray cameraRay;

            if (antiAliasing)
            {
                float level = 2.0f;
                for (int y_continued = y * level; y_continued < 2 + (level * y); y_continued++)
                {
                    for (int x_continued = x * level; x_continued < 2 + (level * x); x_continued++)
                    {
                        const glm::vec2 normalizedPixelPos{
                            float(x_continued) / windowResolution.x * (2.0f / level) - 1.0f,
                            float(y_continued) / windowResolution.y * (2.0f / level) - 1.0f};
                        const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                        color = color + getFinalColor(scene, bvh, cameraRay);
                        if (bloom)
                        {
                            matrixPixels.at(y * windowResolution.x + x) = getFinalColor(scene, bvh, cameraRay);
                            if (color.x + color.y + color.z > 1)
                                matrixColorsScreen.at(y * windowResolution.x + x) = getFinalColor(scene, bvh, cameraRay);
                            else
                                matrixColorsScreen.at(y * windowResolution.x + x) = glm::vec3((0));
                        }
                    }
                }
                color = color / (level * 2.5f);
                screen.setPixel(x, y, color);
            }
            else
            {
                // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
                const glm::vec2 normalizedPixelPos{
                    float(x) / windowResolution.x * 2.0f - 1.0f,
                    float(y) / windowResolution.y * 2.0f - 1.0f};
                cameraRay = camera.generateRay(normalizedPixelPos);
                color = getFinalColor(scene, bvh, cameraRay);
                screen.setPixel(x, y, color);

                if (bloom)
                {
                    matrixPixels.at(y * windowResolution.x + x) = getFinalColor(scene, bvh, cameraRay);
                    if (color.x + color.y + color.z > 1)
                        matrixColorsScreen.at(y * windowResolution.x + x) = getFinalColor(scene, bvh, cameraRay);
                    else
                        matrixColorsScreen.at(y * windowResolution.x + x) = glm::vec3((0));
                }
            }
        }
    }
    //mean over pixels 20x20
    //https://developer.nvidia.com/gpugems/gpugems/part-iv-image-processing/chapter-21-real-time-glow

    if (bloom)
    {
        bloomEffect(matrixPixels, matrixColorsScreen, screen, scene, camera, bvh);
    }
    if (blur)
    {
        blurEffect(scene, camera, bvh, screen, matrixPixels);
    }
}

int main(int argc, char **argv)
{
    Trackball::printHelp();
    std::cout << "\n Press the [R] key on your keyboard to create a ray towards the mouse cursor" << std::endl
              << std::endl;

    Window window{"Final Project - Part 2", windowResolution, OpenGLVersion::GL2};
    Screen screen{windowResolution};
    Trackball camera{&window, glm::radians(50.0f), 3.0f};
    camera.setCamera(glm::vec3(0.0f, 0.0f, 0.0f), glm::radians(glm::vec3(20.0f, 20.0f, 0.0f)), 3.0f);

    SceneType sceneType{SceneType::SingleTriangle};
    std::optional<Ray> optDebugRay;
    Scene scene = loadScene(sceneType, dataPath);
    BoundingVolumeHierarchy bvh{&scene};

    int bvhDebugLevel = 0;
    bool debugBVH{false};
    ViewMode viewMode{ViewMode::Rasterization};

    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
            case GLFW_KEY_R:
            {
                // Shoot a ray. Produce a ray from camera to the far plane.
                const auto tmp = window.getNormalizedCursorPos();
                optDebugRay = camera.generateRay(tmp * 2.0f - 1.0f);
                viewMode = ViewMode::Rasterization;
            }
            break;
            case GLFW_KEY_ESCAPE:
            {
                window.close();
            }
            break;
            };
        }
    });

    int selectedLight{0};
    while (!window.shouldClose())
    {
        window.updateInput();

        // === Setup the UI ===
        ImGui::Begin("Final Project - Part 2");
        {
            constexpr std::array items{"SingleTriangle", "Cube", "Cornell Box (with mirror)", "Cornell Box (spherical light and mirror)", "Monkey", "Dragon", /* "AABBs",*/ "Spheres", /*"Mixed",*/ "Custom"};
            if (ImGui::Combo("Scenes", reinterpret_cast<int *>(&sceneType), items.data(), int(items.size())))
            {
                optDebugRay.reset();
                scene = loadScene(sceneType, dataPath);
                bvh = BoundingVolumeHierarchy(&scene);
                if (optDebugRay)
                {
                    HitInfo dummy{};
                    bvh.intersect(*optDebugRay, dummy);
                }
            }
        }
        {
            constexpr std::array items{"Rasterization", "Ray Traced"};
            ImGui::Combo("View mode", reinterpret_cast<int *>(&viewMode), items.data(), int(items.size()));
        }
        if (ImGui::Button("Render to file"))
        {
            {
                using clock = std::chrono::high_resolution_clock;
                const auto start = clock::now();
                renderRayTracing(scene, camera, bvh, screen);
                const auto end = clock::now();
                std::cout << "Time to render image: " << std::chrono::duration<float, std::milli>(end - start).count() << " milliseconds" << std::endl;
            }
            screen.writeBitmapToFile(outputPath / "render.bmp");
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Debugging");
        if (viewMode == ViewMode::Rasterization)
        {
            ImGui::Checkbox("Draw BVH", &debugBVH);
            if (debugBVH)
                ImGui::SliderInt("BVH Level", &bvhDebugLevel, 0, bvh.numLevels() - 1);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Lights");
        if (!scene.pointLights.empty() || !scene.sphericalLight.empty())
        {
            {
                std::vector<std::string> options;
                for (size_t i = 0; i < scene.pointLights.size(); i++)
                {
                    options.push_back("Point Light " + std::to_string(i + 1));
                }
                for (size_t i = 0; i < scene.sphericalLight.size(); i++)
                {
                    options.push_back("Spherical Light " + std::to_string(i + 1));
                }

                std::vector<const char *> optionsPointers;
                std::transform(std::begin(options), std::end(options), std::back_inserter(optionsPointers),
                               [](const auto &str) { return str.c_str(); });

                ImGui::Combo("Selected light", &selectedLight, optionsPointers.data(), static_cast<int>(optionsPointers.size()));
            }

            {
                const auto showLightOptions = [](auto &light) {
                    ImGui::DragFloat3("Light position", glm::value_ptr(light.position), 0.01f, -3.0f, 3.0f);
                    ImGui::ColorEdit3("Light color", glm::value_ptr(light.color));
                    if constexpr (std::is_same_v<std::decay_t<decltype(light)>, SphericalLight>)
                    {
                        ImGui::DragFloat("Light radius", &light.radius, 0.01f, 0.01f, 0.5f);
                    }
                };
                if (selectedLight < static_cast<int>(scene.pointLights.size()))
                {
                    // Draw a big yellow sphere and then the small light sphere on top.
                    showLightOptions(scene.pointLights[selectedLight]);
                }
                else
                {
                    // Draw a big yellow sphere and then the smaller light sphere on top.
                    showLightOptions(scene.sphericalLight[selectedLight - scene.pointLights.size()]);
                }
            }
        }

        if (ImGui::Button("Add point light"))
        {
            scene.pointLights.push_back(PointLight{glm::vec3(0.0f), glm::vec3(1.0f)});
            selectedLight = int(scene.pointLights.size() - 1);
        }
        if (ImGui::Button("Add spherical light"))
        {
            scene.sphericalLight.push_back(SphericalLight{glm::vec3(0.0f), 0.1f, glm::vec3(1.0f)});
            selectedLight = int(scene.pointLights.size() + scene.sphericalLight.size() - 1);
        }
        if (ImGui::Button("Remove selected light"))
        {
            if (selectedLight < static_cast<int>(scene.pointLights.size()))
            {
                scene.pointLights.erase(std::begin(scene.pointLights) + selectedLight);
            }
            else
            {
                scene.sphericalLight.erase(std::begin(scene.sphericalLight) + (selectedLight - scene.pointLights.size()));
            }
            selectedLight = 0;
        }

        (ImGui::Checkbox("Add Anti Aliasing", &antiAliasing));

        (ImGui::Checkbox("Add bloom", &bloom));

        (ImGui::Checkbox("Add motion blur", &blur));

        // Clear screen.
        glClearDepth(1.0f);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw either using OpenGL (rasterization) or the ray tracing function.
        switch (viewMode)
        {
        case ViewMode::Rasterization:
        {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            renderOpenGL(scene, camera, selectedLight);
            if (optDebugRay)
            {
                // Call getFinalColor for the debug ray. Ignore the result but tell the function that it should
                // draw the rays instead.
                enableDrawRay = true;
                (void)getFinalColor(scene, bvh, *optDebugRay);
                enableDrawRay = false;
            }
            glPopAttrib();
        }
        break;
        case ViewMode::RayTracing:
        {
            screen.clear(glm::vec3(0.0f));
            renderRayTracing(scene, camera, bvh, screen);
            screen.setPixel(0, 0, glm::vec3(1.0f));
            screen.draw(); // Takes the image generated using ray tracing and outputs it to the screen using OpenGL.
        }
        break;
        default:
            break;
        };

        if (debugBVH)
        {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            setOpenGLMatrices(camera);
            glDisable(GL_LIGHTING);
            glEnable(GL_DEPTH_TEST);

            // Enable alpha blending. More info at:
            // https://learnopengl.com/Advanced-OpenGL/Blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            bvh.debugDraw(bvhDebugLevel);
            glPopAttrib();
        }

        ImGui::End();
        window.swapBuffers();
    }

    return 0; // execution never reaches this point
}

static void setOpenGLMatrices(const Trackball &camera)
{
    // Load view matrix.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const glm::mat4 viewMatrix = camera.viewMatrix();
    glMultMatrixf(glm::value_ptr(viewMatrix));

    // Load projection matrix.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const glm::mat4 projectionMatrix = camera.projectionMatrix();
    glMultMatrixf(glm::value_ptr(projectionMatrix));
}

static void renderOpenGL(const Scene &scene, const Trackball &camera, int selectedLight)
{
    // Normals will be normalized in the graphics pipeline.
    glEnable(GL_NORMALIZE);
    // Activate rendering modes.
    glEnable(GL_DEPTH_TEST);
    // Draw front and back facing triangles filled.
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    // Interpolate vertex colors over the triangles.
    glShadeModel(GL_SMOOTH);
    setOpenGLMatrices(camera);

    glDisable(GL_LIGHTING);
    // Render point lights as very small dots
    for (const auto &light : scene.pointLights)
        drawSphere(light.position, 0.01f, light.color);
    for (const auto &light : scene.sphericalLight)
        drawSphere(light.position, light.radius, light.color);

    if (!scene.pointLights.empty() || !scene.sphericalLight.empty())
    {
        if (selectedLight < static_cast<int>(scene.pointLights.size()))
        {
            // Draw a big yellow sphere and then the small light sphere on top.
            const auto &light = scene.pointLights[selectedLight];
            drawSphere(light.position, 0.05f, glm::vec3(1, 1, 0));
            glDisable(GL_DEPTH_TEST);
            drawSphere(light.position, 0.01f, light.color);
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            // Draw a big yellow sphere and then the smaller light sphere on top.
            const auto &light = scene.sphericalLight[selectedLight - scene.pointLights.size()];
            drawSphere(light.position, light.radius + 0.01f, glm::vec3(1, 1, 0));
            glDisable(GL_DEPTH_TEST);
            drawSphere(light.position, light.radius, light.color);
            glEnable(GL_DEPTH_TEST);
        }
    }

    // Activate the light in the legacy OpenGL mode.
    glEnable(GL_LIGHTING);

    int i = 0;
    const auto enableLight = [&](const auto &light) {
        glEnable(GL_LIGHT0 + i);
        const glm::vec4 position4{light.position, 1};
        glLightfv(GL_LIGHT0 + i, GL_POSITION, glm::value_ptr(position4));
        const glm::vec4 color4{glm::clamp(light.color, 0.0f, 1.0f), 1.0f};
        const glm::vec4 zero4{0.0f, 0.0f, 0.0f, 1.0f};
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, glm::value_ptr(zero4));
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, glm::value_ptr(color4));
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, glm::value_ptr(zero4));
        // NOTE: quadratic attenuation doesn't work like you think it would in legacy OpenGL.
        // The distance is not in world space but in NDC space!
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.0f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.0f);
        i++;
    };
    for (const auto &light : scene.pointLights)
        enableLight(light);
    for (const auto &light : scene.sphericalLight)
        enableLight(light);

    // Draw the scene and the ray (if any).
    drawScene(scene);

    // Draw a colored sphere at the location at which the trackball is looking/rotating around.
    glDisable(GL_LIGHTING);
    drawSphere(camera.lookAt(), 0.01f, glm::vec3(0.2f, 0.2f, 1.0f));
}
