#include <gst/gst.h>
#include <gst/gstobject.h>
#include "vid_render/TVidRender.hpp"

using namespace gentau;

std::vector<GstPluginFeature*> m_blockedFeatures;

void blockFeature(const char* feature_name)
{
	GstRegistry*      registry = gst_registry_get();
	GstPluginFeature* feature  = gst_registry_lookup_feature(registry, feature_name);
	if (feature) {
		// 设置到极低的优先级或从注册表移除，强制 gst_element_factory_make 失败
		gst_registry_remove_feature(registry, feature);
		m_blockedFeatures.push_back(feature);
	}
}

void TearDown()
{
	// 恢复被屏蔽的插件，避免影响其他测试
	GstRegistry* registry = gst_registry_get();
	for (auto* f : m_blockedFeatures) {
		gst_registry_add_feature(registry, f);
		gst_object_unref(f);
	}
	m_blockedFeatures.clear();
}

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);

	blockFeature("qml6glsink");

	for (int i = 0; i < 1000; i++) {
		try {
			TVidRender render("random_path");
		} catch (const std::exception& e) {
			continue;
		}
	}

    TearDown();
    return 0;
}

