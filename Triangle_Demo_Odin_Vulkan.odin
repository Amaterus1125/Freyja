// Same gradient triangle (red/green/blue corners, hardcoded in the vertex
// shader) as the C++ Vulkan version -- this is a straight structural port:
// instance -> device -> swapchain -> render pass -> pipeline -> draw loop.
//
// Reuses the SAME shader.vert / shader.frag from the C++ project. Compile
// them to SPIR-V and place next to the executable as:
//   shaders/shader.vert.spv
//   shaders/shader.frag.spv
//
//   glslc shader.vert -o shaders/shader.vert.spv
//   glslc shader.frag -o shaders/shader.frag.spv
//
// (glslc ships with the Vulkan SDK -- same requirement as the C++ build.)
//
// Run with:  odin run .


package main

import "core:fmt"
import "core:os"
import "base:runtime"
import "vendor:glfw"
import vk "vendor:vulkan"

WIDTH :: 800
HEIGHT :: 600
MAX_FRAMES_IN_FLIGHT :: 2

ENABLE_VALIDATION :: true // flip to false for a release build

validation_layers := [1]cstring{"VK_LAYER_KHRONOS_validation"}


// Global state -- mirrors the member variables of the C++ class.

window:              glfw.WindowHandle
instance:            vk.Instance
debug_messenger:     vk.DebugUtilsMessengerEXT
surface:             vk.SurfaceKHR

physical_device:     vk.PhysicalDevice
device:              vk.Device
graphics_queue:      vk.Queue
present_queue:       vk.Queue

swap_chain:              vk.SwapchainKHR
swap_chain_images:       []vk.Image
swap_chain_format:       vk.Format
swap_chain_extent:       vk.Extent2D
swap_chain_views:        []vk.ImageView
swap_chain_framebuffers: []vk.Framebuffer

render_pass:       vk.RenderPass
pipeline_layout:   vk.PipelineLayout
graphics_pipeline: vk.Pipeline

command_pool:    vk.CommandPool
command_buffers: []vk.CommandBuffer

image_available_sem: [MAX_FRAMES_IN_FLIGHT]vk.Semaphore
render_finished_sem: [MAX_FRAMES_IN_FLIGHT]vk.Semaphore
in_flight_fences:    [MAX_FRAMES_IN_FLIGHT]vk.Fence
current_frame:       u32
framebuffer_resized: bool

Queue_Family_Indices :: struct {
	graphics_family: Maybe(u32),
	present_family:  Maybe(u32),
}

Swap_Chain_Support :: struct {
	capabilities:  vk.SurfaceCapabilitiesKHR,
	formats:       []vk.SurfaceFormatKHR,
	present_modes: []vk.PresentModeKHR,
}


main :: proc() {
	init_window()
	init_vulkan()
	main_loop()
	cleanup()
}

read_file :: proc(path: string) -> []byte {
	data, ok := os.read_entire_file(path)
	if !ok {
		fmt.eprintln("failed to open file:", path)
		os.exit(1)
	}
	return data
}

debug_callback :: proc "system" (
	messageSeverity: vk.DebugUtilsMessageSeverityFlagsEXT,
	messageType: vk.DebugUtilsMessageTypeFlagsEXT,
	pCallbackData: ^vk.DebugUtilsMessengerCallbackDataEXT,
	pUserData: rawptr,
) -> b32 {
	context = runtime.default_context()
	fmt.eprintln("[validation layer]", pCallbackData.pMessage)
	return false
}


framebuffer_resize_callback :: proc "c" (win: glfw.WindowHandle, width, height: i32) {
	framebuffer_resized = true
}

init_window :: proc() {
	glfw.Init()
	glfw.WindowHint(glfw.CLIENT_API, glfw.NO_API) // no OpenGL context
	window = glfw.CreateWindow(WIDTH, HEIGHT, "Vulkan Triangle - Odin", nil, nil)
	glfw.SetFramebufferSizeCallback(window, framebuffer_resize_callback)
}


init_vulkan :: proc() {
	vk.load_proc_addresses_global(auto_cast glfw.GetInstanceProcAddress)

	create_instance()
	vk.load_proc_addresses_instance(instance)

	setup_debug_messenger()
	create_surface()
	pick_physical_device()
	create_logical_device()
	vk.load_proc_addresses_device(device)

	create_swap_chain()
	create_image_views()
	create_render_pass()
	create_graphics_pipeline()
	create_framebuffers()
	create_command_pool()
	create_command_buffers()
	create_sync_objects()
}

main_loop :: proc() {
	for !glfw.WindowShouldClose(window) {
		glfw.PollEvents()
		draw_frame()
	}
	vk.DeviceWaitIdle(device)
}

cleanup :: proc() {
	cleanup_swap_chain()

	vk.DestroyPipeline(device, graphics_pipeline, nil)
	vk.DestroyPipelineLayout(device, pipeline_layout, nil)
	vk.DestroyRenderPass(device, render_pass, nil)

	for i in 0 ..< MAX_FRAMES_IN_FLIGHT {
		vk.DestroySemaphore(device, render_finished_sem[i], nil)
		vk.DestroySemaphore(device, image_available_sem[i], nil)
		vk.DestroyFence(device, in_flight_fences[i], nil)
	}

	vk.DestroyCommandPool(device, command_pool, nil)
	vk.DestroyDevice(device, nil)

	if ENABLE_VALIDATION {
		vk.DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nil)
	}

	vk.DestroySurfaceKHR(instance, surface, nil)
	vk.DestroyInstance(instance, nil)

	glfw.DestroyWindow(window)
	glfw.Terminate()
}


// Instance + validation layers.

check_validation_layer_support :: proc() -> bool {
	count: u32
	vk.EnumerateInstanceLayerProperties(&count, nil)
	available := make([]vk.LayerProperties, count, context.temp_allocator)
	vk.EnumerateInstanceLayerProperties(&count, raw_data(available))

	for layer_name in validation_layers {
		found := false
		for layer in available {
			name := cstring(&layer.layerName[0])
			if name == layer_name {
				found = true
				break
			}
		}
		if !found do return false
	}
	return true
}

populate_debug_messenger_create_info :: proc(info: ^vk.DebugUtilsMessengerCreateInfoEXT) {
	info^ = vk.DebugUtilsMessengerCreateInfoEXT {
		sType           = .DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		messageSeverity = {.VERBOSE, .WARNING, .ERROR},
		messageType     = {.GENERAL, .VALIDATION, .PERFORMANCE},
		pfnUserCallback = debug_callback,
	}
}

create_instance :: proc() {
	if ENABLE_VALIDATION && !check_validation_layer_support() {
		fmt.eprintln("validation layers requested, but not available")
		os.exit(1)
	}

	app_info := vk.ApplicationInfo {
		sType              = .APPLICATION_INFO,
		pApplicationName   = "Vulkan Triangle",
		applicationVersion = 1,
		pEngineName        = "No Engine",
		engineVersion      = 1,
		apiVersion         = vk.API_VERSION_1_0,
	}

	extensions := make([dynamic]cstring, context.temp_allocator)
	for ext in glfw.GetRequiredInstanceExtensions() {
		append(&extensions, ext)
	}
	if ENABLE_VALIDATION {
		append(&extensions, "VK_EXT_debug_utils")
	}

	create_info := vk.InstanceCreateInfo {
		sType                   = .INSTANCE_CREATE_INFO,
		pApplicationInfo        = &app_info,
		enabledExtensionCount   = u32(len(extensions)),
		ppEnabledExtensionNames = raw_data(extensions[:]),
	}

	debug_create_info: vk.DebugUtilsMessengerCreateInfoEXT
	if ENABLE_VALIDATION {
		create_info.enabledLayerCount = u32(len(validation_layers))
		create_info.ppEnabledLayerNames = raw_data(validation_layers[:])

		populate_debug_messenger_create_info(&debug_create_info)
		create_info.pNext = &debug_create_info
	}

	if vk.CreateInstance(&create_info, nil, &instance) != .SUCCESS {
		fmt.eprintln("failed to create instance")
		os.exit(1)
	}
}

setup_debug_messenger :: proc() {
	if !ENABLE_VALIDATION do return

	create_info: vk.DebugUtilsMessengerCreateInfoEXT
	populate_debug_messenger_create_info(&create_info)

	if vk.CreateDebugUtilsMessengerEXT(instance, &create_info, nil, &debug_messenger) != .SUCCESS {
		fmt.eprintln("failed to set up debug messenger")
		os.exit(1)
	}
}

create_surface :: proc() {
	if glfw.CreateWindowSurface(instance, window, nil, &surface) != .SUCCESS {
		fmt.eprintln("failed to create window surface")
		os.exit(1)
	}
}

// ----------------------------------------------------------------------------
// Physical / logical device.
// ----------------------------------------------------------------------------
queue_family_indices_complete :: proc(qi: Queue_Family_Indices) -> bool {
	return qi.graphics_family != nil && qi.present_family != nil
}

find_queue_families :: proc(dev: vk.PhysicalDevice) -> Queue_Family_Indices {
	indices: Queue_Family_Indices

	count: u32
	vk.GetPhysicalDeviceQueueFamilyProperties(dev, &count, nil)
	families := make([]vk.QueueFamilyProperties, count, context.temp_allocator)
	vk.GetPhysicalDeviceQueueFamilyProperties(dev, &count, raw_data(families))

	for family, i in families {
		if .GRAPHICS in family.queueFlags {
			indices.graphics_family = u32(i)
		}

		present_support: b32
		vk.GetPhysicalDeviceSurfaceSupportKHR(dev, u32(i), surface, &present_support)
		if present_support {
			indices.present_family = u32(i)
		}

		if queue_family_indices_complete(indices) do break
	}
	return indices
}

check_device_extension_support :: proc(dev: vk.PhysicalDevice) -> bool {
	count: u32
	vk.EnumerateDeviceExtensionProperties(dev, nil, &count, nil)
	available := make([]vk.ExtensionProperties, count, context.temp_allocator)
	vk.EnumerateDeviceExtensionProperties(dev, nil, &count, raw_data(available))

	required := [1]cstring{"VK_KHR_swapchain"}
	for req in required {
		found := false
		for ext in available {
			name := cstring(&ext.extensionName[0])
			if name == req {
				found = true
				break
			}
		}
		if !found do return false
	}
	return true
}

query_swap_chain_support :: proc(dev: vk.PhysicalDevice) -> Swap_Chain_Support {
	details: Swap_Chain_Support
	vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities)

	format_count: u32
	vk.GetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, nil)
	if format_count != 0 {
		details.formats = make([]vk.SurfaceFormatKHR, format_count)
		vk.GetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, raw_data(details.formats))
	}

	mode_count: u32
	vk.GetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &mode_count, nil)
	if mode_count != 0 {
		details.present_modes = make([]vk.PresentModeKHR, mode_count)
		vk.GetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &mode_count, raw_data(details.present_modes))
	}

	return details
}

is_device_suitable :: proc(dev: vk.PhysicalDevice) -> bool {
	indices := find_queue_families(dev)
	extensions_supported := check_device_extension_support(dev)

	swap_chain_adequate := false
	if extensions_supported {
		support := query_swap_chain_support(dev)
		swap_chain_adequate = len(support.formats) > 0 && len(support.present_modes) > 0
	}

	return queue_family_indices_complete(indices) && extensions_supported && swap_chain_adequate
}

pick_physical_device :: proc() {
	count: u32
	vk.EnumeratePhysicalDevices(instance, &count, nil)
	if count == 0 {
		fmt.eprintln("failed to find GPUs with Vulkan support")
		os.exit(1)
	}
	devices := make([]vk.PhysicalDevice, count, context.temp_allocator)
	vk.EnumeratePhysicalDevices(instance, &count, raw_data(devices))

	for dev in devices {
		if is_device_suitable(dev) {
			physical_device = dev
			break
		}
	}
	if physical_device == nil {
		fmt.eprintln("failed to find a suitable GPU")
		os.exit(1)
	}
}

create_logical_device :: proc() {
	indices := find_queue_families(physical_device)

	unique_families := make(map[u32]bool, context.temp_allocator)
	unique_families[indices.graphics_family.?] = true
	unique_families[indices.present_family.?] = true

	queue_create_infos := make([dynamic]vk.DeviceQueueCreateInfo, context.temp_allocator)
	queue_priority: f32 = 1.0
	for family in unique_families {
		append(&queue_create_infos, vk.DeviceQueueCreateInfo{
			sType            = .DEVICE_QUEUE_CREATE_INFO,
			queueFamilyIndex = family,
			queueCount       = 1,
			pQueuePriorities = &queue_priority,
		})
	}

	device_features := vk.PhysicalDeviceFeatures{}
	device_extensions := [1]cstring{"VK_KHR_swapchain"}

	create_info := vk.DeviceCreateInfo {
		sType                   = .DEVICE_CREATE_INFO,
		queueCreateInfoCount    = u32(len(queue_create_infos)),
		pQueueCreateInfos       = raw_data(queue_create_infos[:]),
		pEnabledFeatures        = &device_features,
		enabledExtensionCount   = u32(len(device_extensions)),
		ppEnabledExtensionNames = raw_data(device_extensions[:]),
	}

	if ENABLE_VALIDATION {
		create_info.enabledLayerCount = u32(len(validation_layers))
		create_info.ppEnabledLayerNames = raw_data(validation_layers[:])
	}

	if vk.CreateDevice(physical_device, &create_info, nil, &device) != .SUCCESS {
		fmt.eprintln("failed to create logical device")
		os.exit(1)
	}

	vk.GetDeviceQueue(device, indices.graphics_family.?, 0, &graphics_queue)
	vk.GetDeviceQueue(device, indices.present_family.?, 0, &present_queue)
}


// Swap chain.

choose_swap_surface_format :: proc(formats: []vk.SurfaceFormatKHR) -> vk.SurfaceFormatKHR {
	for f in formats {
		// NOTE: if this line fails to compile, check the exact enum member
		// name for VK_COLOR_SPACE_SRGB_NONLINEAR_KHR in your Odin's
		// vendor/vulkan source -- naming has shifted before.
		if f.format == .B8G8R8A8_SRGB && f.colorSpace == .SRGB_NONLINEAR {
			return f
		}
	}
	return formats[0]
}

choose_swap_present_mode :: proc(modes: []vk.PresentModeKHR) -> vk.PresentModeKHR {
	for m in modes {
		if m == .MAILBOX do return m
	}
	return .FIFO
}

choose_swap_extent :: proc(capabilities: vk.SurfaceCapabilitiesKHR) -> vk.Extent2D {
	if capabilities.currentExtent.width != max(u32) {
		return capabilities.currentExtent
	}

	w, h := glfw.GetFramebufferSize(window)
	extent := vk.Extent2D{u32(w), u32(h)}
	extent.width = clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width)
	extent.height = clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	return extent
}

create_swap_chain :: proc() {
	support := query_swap_chain_support(physical_device)

	surface_format := choose_swap_surface_format(support.formats)
	present_mode := choose_swap_present_mode(support.present_modes)
	extent := choose_swap_extent(support.capabilities)

	image_count := support.capabilities.minImageCount + 1
	if support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount {
		image_count = support.capabilities.maxImageCount
	}

	indices := find_queue_families(physical_device)
	qfi := [2]u32{indices.graphics_family.?, indices.present_family.?}

	create_info := vk.SwapchainCreateInfoKHR {
		sType            = .SWAPCHAIN_CREATE_INFO_KHR,
		surface          = surface,
		minImageCount    = image_count,
		imageFormat      = surface_format.format,
		imageColorSpace  = surface_format.colorSpace,
		imageExtent      = extent,
		imageArrayLayers = 1,
		imageUsage       = {.COLOR_ATTACHMENT},
		preTransform     = support.capabilities.currentTransform,
		compositeAlpha   = {.OPAQUE},
		presentMode      = present_mode,
		clipped          = true,
	}

	if indices.graphics_family.? != indices.present_family.? {
		create_info.imageSharingMode = .CONCURRENT
		create_info.queueFamilyIndexCount = 2
		create_info.pQueueFamilyIndices = raw_data(qfi[:])
	} else {
		create_info.imageSharingMode = .EXCLUSIVE
	}

	if vk.CreateSwapchainKHR(device, &create_info, nil, &swap_chain) != .SUCCESS {
		fmt.eprintln("failed to create swap chain")
		os.exit(1)
	}

	count: u32
	vk.GetSwapchainImagesKHR(device, swap_chain, &count, nil)
	swap_chain_images = make([]vk.Image, count)
	vk.GetSwapchainImagesKHR(device, swap_chain, &count, raw_data(swap_chain_images))

	swap_chain_format = surface_format.format
	swap_chain_extent = extent
}

cleanup_swap_chain :: proc() {
	for fb in swap_chain_framebuffers {
		vk.DestroyFramebuffer(device, fb, nil)
	}
	delete(swap_chain_framebuffers)

	vk.FreeCommandBuffers(device, command_pool, u32(len(command_buffers)), raw_data(command_buffers))
	delete(command_buffers)

	for view in swap_chain_views {
		vk.DestroyImageView(device, view, nil)
	}
	delete(swap_chain_views)

	vk.DestroySwapchainKHR(device, swap_chain, nil)
	delete(swap_chain_images)
}

recreate_swap_chain :: proc() {
	// Handle minimization by waiting until the window has a real size again.
	w, h := glfw.GetFramebufferSize(window)
	for w == 0 || h == 0 {
		w, h = glfw.GetFramebufferSize(window)
		glfw.WaitEvents()
	}

	vk.DeviceWaitIdle(device)

	cleanup_swap_chain()

	create_swap_chain()
	create_image_views()
	create_framebuffers()
	create_command_buffers()
}

create_image_views :: proc() {
	swap_chain_views = make([]vk.ImageView, len(swap_chain_images))
	for img, i in swap_chain_images {
		create_info := vk.ImageViewCreateInfo {
			sType = .IMAGE_VIEW_CREATE_INFO,
			image = img,
			viewType = .D2,
			format = swap_chain_format,
			components = {r = .IDENTITY, g = .IDENTITY, b = .IDENTITY, a = .IDENTITY},
			subresourceRange = {
				aspectMask = {.COLOR},
				baseMipLevel = 0,
				levelCount = 1,
				baseArrayLayer = 0,
				layerCount = 1,
			},
		}
		if vk.CreateImageView(device, &create_info, nil, &swap_chain_views[i]) != .SUCCESS {
			fmt.eprintln("failed to create image view")
			os.exit(1)
		}
	}
}

// Render pass + pipeline.
create_render_pass :: proc() {
	color_attachment := vk.AttachmentDescription {
		format         = swap_chain_format,
		samples        = {._1},
		loadOp         = .CLEAR,
		storeOp        = .STORE,
		stencilLoadOp  = .DONT_CARE,
		stencilStoreOp = .DONT_CARE,
		initialLayout  = .UNDEFINED,
		finalLayout    = .PRESENT_SRC_KHR,
	}

	color_attachment_ref := vk.AttachmentReference {
		attachment = 0,
		layout     = .COLOR_ATTACHMENT_OPTIMAL,
	}

	subpass := vk.SubpassDescription {
		pipelineBindPoint    = .GRAPHICS,
		colorAttachmentCount = 1,
		pColorAttachments    = &color_attachment_ref,
	}

	dependency := vk.SubpassDependency {
		srcSubpass    = vk.SUBPASS_EXTERNAL,
		dstSubpass    = 0,
		srcStageMask  = {.COLOR_ATTACHMENT_OUTPUT},
		dstStageMask  = {.COLOR_ATTACHMENT_OUTPUT},
		dstAccessMask = {.COLOR_ATTACHMENT_WRITE},
	}

	render_pass_info := vk.RenderPassCreateInfo {
		sType           = .RENDER_PASS_CREATE_INFO,
		attachmentCount = 1,
		pAttachments    = &color_attachment,
		subpassCount    = 1,
		pSubpasses      = &subpass,
		dependencyCount = 1,
		pDependencies   = &dependency,
	}

	if vk.CreateRenderPass(device, &render_pass_info, nil, &render_pass) != .SUCCESS {
		fmt.eprintln("failed to create render pass")
		os.exit(1)
	}
}

create_shader_module :: proc(code: []byte) -> vk.ShaderModule {
	create_info := vk.ShaderModuleCreateInfo {
		sType    = .SHADER_MODULE_CREATE_INFO,
		codeSize = len(code),
		pCode    = cast(^u32)raw_data(code),
	}
	module: vk.ShaderModule
	if vk.CreateShaderModule(device, &create_info, nil, &module) != .SUCCESS {
		fmt.eprintln("failed to create shader module")
		os.exit(1)
	}
	return module
}

create_graphics_pipeline :: proc() {
	vert_code := read_file("shaders/shader.vert.spv")
	frag_code := read_file("shaders/shader.frag.spv")
	defer delete(vert_code)
	defer delete(frag_code)

	vert_module := create_shader_module(vert_code)
	frag_module := create_shader_module(frag_code)
	defer vk.DestroyShaderModule(device, vert_module, nil)
	defer vk.DestroyShaderModule(device, frag_module, nil)

	stages := [2]vk.PipelineShaderStageCreateInfo {
		{sType = .PIPELINE_SHADER_STAGE_CREATE_INFO, stage = {.VERTEX}, module = vert_module, pName = "main"},
		{sType = .PIPELINE_SHADER_STAGE_CREATE_INFO, stage = {.FRAGMENT}, module = frag_module, pName = "main"},
	}

	// No vertex buffers -- positions/colors are hardcoded in the vertex shader.
	vertex_input := vk.PipelineVertexInputStateCreateInfo{sType = .PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO}

	input_assembly := vk.PipelineInputAssemblyStateCreateInfo {
		sType    = .PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		topology = .TRIANGLE_LIST,
	}

	dynamic_states := [2]vk.DynamicState{.VIEWPORT, .SCISSOR}
	dynamic_state := vk.PipelineDynamicStateCreateInfo {
		sType             = .PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		dynamicStateCount = 2,
		pDynamicStates    = raw_data(dynamic_states[:]),
	}

	viewport_state := vk.PipelineViewportStateCreateInfo {
		sType         = .PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		viewportCount = 1,
		scissorCount  = 1,
	}

	rasterizer := vk.PipelineRasterizationStateCreateInfo {
		sType       = .PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		polygonMode = .FILL,
		lineWidth   = 1.0,
		cullMode    = {.BACK},
		frontFace   = .CLOCKWISE,
	}

	multisampling := vk.PipelineMultisampleStateCreateInfo {
		sType                = .PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		rasterizationSamples = {._1},
	}

	color_blend_attachment := vk.PipelineColorBlendAttachmentState {
		colorWriteMask = {.R, .G, .B, .A},
		blendEnable    = false,
	}

	color_blending := vk.PipelineColorBlendStateCreateInfo {
		sType           = .PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		attachmentCount = 1,
		pAttachments    = &color_blend_attachment,
	}

	layout_info := vk.PipelineLayoutCreateInfo{sType = .PIPELINE_LAYOUT_CREATE_INFO}
	if vk.CreatePipelineLayout(device, &layout_info, nil, &pipeline_layout) != .SUCCESS {
		fmt.eprintln("failed to create pipeline layout")
		os.exit(1)
	}

	pipeline_info := vk.GraphicsPipelineCreateInfo {
		sType               = .GRAPHICS_PIPELINE_CREATE_INFO,
		stageCount          = 2,
		pStages             = raw_data(stages[:]),
		pVertexInputState   = &vertex_input,
		pInputAssemblyState = &input_assembly,
		pViewportState      = &viewport_state,
		pRasterizationState = &rasterizer,
		pMultisampleState   = &multisampling,
		pColorBlendState    = &color_blending,
		pDynamicState       = &dynamic_state,
		layout              = pipeline_layout,
		renderPass          = render_pass,
		subpass             = 0,
	}

	if vk.CreateGraphicsPipelines(device, {}, 1, &pipeline_info, nil, &graphics_pipeline) != .SUCCESS {
		fmt.eprintln("failed to create graphics pipeline")
		os.exit(1)
	}
}

create_framebuffers :: proc() {
	swap_chain_framebuffers = make([]vk.Framebuffer, len(swap_chain_views))
	for view, i in swap_chain_views {
		attachments := [1]vk.ImageView{view}
		fb_info := vk.FramebufferCreateInfo {
			sType           = .FRAMEBUFFER_CREATE_INFO,
			renderPass      = render_pass,
			attachmentCount = 1,
			pAttachments    = raw_data(attachments[:]),
			width           = swap_chain_extent.width,
			height          = swap_chain_extent.height,
			layers          = 1,
		}
		if vk.CreateFramebuffer(device, &fb_info, nil, &swap_chain_framebuffers[i]) != .SUCCESS {
			fmt.eprintln("failed to create framebuffer")
			os.exit(1)
		}
	}
}


// Commands + sync.

create_command_pool :: proc() {
	indices := find_queue_families(physical_device)
	pool_info := vk.CommandPoolCreateInfo {
		sType            = .COMMAND_POOL_CREATE_INFO,
		flags            = {.RESET_COMMAND_BUFFER},
		queueFamilyIndex = indices.graphics_family.?,
	}
	if vk.CreateCommandPool(device, &pool_info, nil, &command_pool) != .SUCCESS {
		fmt.eprintln("failed to create command pool")
		os.exit(1)
	}
}

create_command_buffers :: proc() {
	command_buffers = make([]vk.CommandBuffer, len(swap_chain_framebuffers))
	alloc_info := vk.CommandBufferAllocateInfo {
		sType              = .COMMAND_BUFFER_ALLOCATE_INFO,
		commandPool        = command_pool,
		level              = .PRIMARY,
		commandBufferCount = u32(len(command_buffers)),
	}
	if vk.AllocateCommandBuffers(device, &alloc_info, raw_data(command_buffers)) != .SUCCESS {
		fmt.eprintln("failed to allocate command buffers")
		os.exit(1)
	}
}

record_command_buffer :: proc(cmd: vk.CommandBuffer, image_index: u32) {
	begin_info := vk.CommandBufferBeginInfo{sType = .COMMAND_BUFFER_BEGIN_INFO}
	if vk.BeginCommandBuffer(cmd, &begin_info) != .SUCCESS {
		fmt.eprintln("failed to begin recording command buffer")
		os.exit(1)
	}

	clear_color := vk.ClearValue{color = {float32 = {0.01, 0.01, 0.02, 1.0}}}

	render_pass_info := vk.RenderPassBeginInfo {
		sType           = .RENDER_PASS_BEGIN_INFO,
		renderPass      = render_pass,
		framebuffer     = swap_chain_framebuffers[image_index],
		renderArea      = {offset = {0, 0}, extent = swap_chain_extent},
		clearValueCount = 1,
		pClearValues    = &clear_color,
	}

	vk.CmdBeginRenderPass(cmd, &render_pass_info, .INLINE)
	vk.CmdBindPipeline(cmd, .GRAPHICS, graphics_pipeline)

	viewport := vk.Viewport {
		width    = f32(swap_chain_extent.width),
		height   = f32(swap_chain_extent.height),
		maxDepth = 1.0,
	}
	vk.CmdSetViewport(cmd, 0, 1, &viewport)

	scissor := vk.Rect2D{offset = {0, 0}, extent = swap_chain_extent}
	vk.CmdSetScissor(cmd, 0, 1, &scissor)

	vk.CmdDraw(cmd, 3, 1, 0, 0) // 3 hardcoded vertices, 1 instance

	vk.CmdEndRenderPass(cmd)

	if vk.EndCommandBuffer(cmd) != .SUCCESS {
		fmt.eprintln("failed to record command buffer")
		os.exit(1)
	}
}

create_sync_objects :: proc() {
	sem_info := vk.SemaphoreCreateInfo{sType = .SEMAPHORE_CREATE_INFO}
	fence_info := vk.FenceCreateInfo{sType = .FENCE_CREATE_INFO, flags = {.SIGNALED}}

	for i in 0 ..< MAX_FRAMES_IN_FLIGHT {
		if vk.CreateSemaphore(device, &sem_info, nil, &image_available_sem[i]) != .SUCCESS ||
		   vk.CreateSemaphore(device, &sem_info, nil, &render_finished_sem[i]) != .SUCCESS ||
		   vk.CreateFence(device, &fence_info, nil, &in_flight_fences[i]) != .SUCCESS {
			fmt.eprintln("failed to create synchronization objects for a frame")
			os.exit(1)
		}
	}
}

// ----------------------------------------------------------------------------
// Per-frame draw loop.
// ----------------------------------------------------------------------------
draw_frame :: proc() {
	vk.WaitForFences(device, 1, &in_flight_fences[current_frame], true, max(u64))

	image_index: u32
	result := vk.AcquireNextImageKHR(device, swap_chain, max(u64), image_available_sem[current_frame], {}, &image_index)

	if result == .ERROR_OUT_OF_DATE_KHR {
		recreate_swap_chain()
		return
	} else if result != .SUCCESS && result != .SUBOPTIMAL_KHR {
		fmt.eprintln("failed to acquire swap chain image")
		os.exit(1)
	}

	vk.ResetFences(device, 1, &in_flight_fences[current_frame])

	vk.ResetCommandBuffer(command_buffers[image_index], {})
	record_command_buffer(command_buffers[image_index], image_index)

	wait_stage := vk.PipelineStageFlags{.COLOR_ATTACHMENT_OUTPUT}
	submit_info := vk.SubmitInfo {
		sType                = .SUBMIT_INFO,
		waitSemaphoreCount   = 1,
		pWaitSemaphores      = &image_available_sem[current_frame],
		pWaitDstStageMask    = &wait_stage,
		commandBufferCount   = 1,
		pCommandBuffers      = &command_buffers[image_index],
		signalSemaphoreCount = 1,
		pSignalSemaphores    = &render_finished_sem[current_frame],
	}

	if vk.QueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != .SUCCESS {
		fmt.eprintln("failed to submit draw command buffer")
		os.exit(1)
	}

	present_info := vk.PresentInfoKHR {
		sType              = .PRESENT_INFO_KHR,
		waitSemaphoreCount = 1,
		pWaitSemaphores    = &render_finished_sem[current_frame],
		swapchainCount     = 1,
		pSwapchains        = &swap_chain,
		pImageIndices      = &image_index,
	}

	result = vk.QueuePresentKHR(present_queue, &present_info)

	if result == .ERROR_OUT_OF_DATE_KHR || result == .SUBOPTIMAL_KHR || framebuffer_resized {
		framebuffer_resized = false
		recreate_swap_chain()
	} else if result != .SUCCESS {
		fmt.eprintln("failed to present swap chain image")
		os.exit(1)
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT
}
