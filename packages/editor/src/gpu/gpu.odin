package gpu

import "base:runtime"
import "core:fmt"
import "vendor:wgpu"

Context :: struct {
    instance:       wgpu.Instance,
    surface:        wgpu.Surface,
    surface_config: wgpu.SurfaceConfiguration,
    device:         wgpu.Device,
    encoder:        wgpu.CommandEncoder,
    adapter:        wgpu.Adapter,
    queue:          wgpu.Queue,
}

@(private = "file")
Internal :: struct {
    gpu:      ^Context,
    ctx:      runtime.Context,
    ready:    proc(userdata: rawptr),
    userdata: rawptr,
}

init :: proc(
    self: ^Context,
    instance: wgpu.Instance,
    surface: wgpu.Surface,
    ready: proc(userdata: rawptr),
    userdata: rawptr,
) {
    self.instance = instance
    self.surface = surface

    internal := Internal {
        gpu      = self,
        ctx      = context,
        ready    = ready,
        userdata = userdata,
    }

    wgpu.InstanceRequestAdapter(
        self.instance,
        &{compatibleSurface = self.surface},
        {callback = on_adapter, userdata1 = &internal},
    )

    on_adapter :: proc "c" (
        status: wgpu.RequestAdapterStatus,
        adapter: wgpu.Adapter,
        message: string,
        userdata1: rawptr,
        userdata2: rawptr,
    ) {
        internal := cast(^Internal)userdata1
        context = internal.ctx
        gpu := internal.gpu

        if status != .Success || adapter == nil {
            fmt.panicf("request adapter failure: [%v] %s", status, message)
        }

        gpu.adapter = adapter
        wgpu.AdapterRequestDevice(
            adapter,
            nil,
            {callback = on_device, userdata1 = internal},
        )
    }

    on_device :: proc "c" (
        status: wgpu.RequestDeviceStatus,
        device: wgpu.Device,
        message: wgpu.StringView,
        userdata1: rawptr,
        userdata2: rawptr,
    ) {
        internal := cast(^Internal)userdata1
        context = internal.ctx
        gpu := internal.gpu

        if status != .Success || device == nil {
            fmt.panicf("request device failure: [%v] %s", status, message)
        }
        gpu.device = device

        gpu.surface_config = wgpu.SurfaceConfiguration {
            width       = 800,
            height      = 600,
            device      = gpu.device,
            usage       = {.RenderAttachment},
            format      = PREFERED_TEXTURE_FORMAT,
            presentMode = .Fifo,
            alphaMode   = .Opaque,
        }

        wgpu.SurfaceConfigure(gpu.surface, &gpu.surface_config)
        gpu.queue = wgpu.DeviceGetQueue(gpu.device)

        if internal.ready != nil {
            internal.ready(internal.userdata)
        }
    }
}

uninit :: proc(self: ^Context) {
    wgpu.QueueRelease(self.queue)
    wgpu.DeviceRelease(self.device)
    wgpu.AdapterRelease(self.adapter)
    wgpu.SurfaceRelease(self.surface)
    wgpu.InstanceRelease(self.instance)
}

resize_surface :: proc(self: ^Context, width: u32, height: u32) {
    self.surface_config.width = width
    self.surface_config.height = height
    wgpu.SurfaceConfigure(self.surface, &self.surface_config)
}

