#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "net.h"

/* cueue head of devices */
/* NOTE: if you want to add/delete the entries after net_run(), you need to protect these lists with a mutex. */
static struct net_device *devices;

struct net_device *
net_device_alloc(void)
{
  struct net_device *dev;

  dev = calloc(1, sizeof(*dev));
  if (!dev)
  {
    errorf("calloc() failure");
    return NULL;
  }
  return dev;
}

/* NOTE: must not be called after net_run() */
int net_device_register(struct net_device *dev)
{
  static unsigned int index = 0;

  dev->index = index++; /* incremantation is evaluated after assignation */
  snprintf(dev->name, sizeof(dev->name), "net%d", dev->index);
  dev->next = devices;
  devices = dev;
  infof("regiseterd, dev=$s, type=0x%04x", dev->name, dev->type);
  return 0;
}

static int
net_device_open(struct net_device *dev)
{
  if (NET_DEVICE_IS_UP(dev))
  {
    errorf("device is already open, dev=%s", dev->name);
    return -1;
  }
  if (dev->ops->open)
  {
    if (dev->ops->open(dev) == -1)
    {
      errorf("failure, dev=%s", dev->name);
      return -1;
    }
  }
  dev->flags |= NET_DEVICE_FLAG_UP;
  infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
  return 0;
}

static int
net_device_close(struct net_device *dev)
{
  if (!NET_DEVICE_IS_UP(dev))
  {
    errorf("not opened, dev=%s", dev->name);
    return -1;
  }
  if (dev->ops->close)
  {
    if (dev->ops->close(dev) == -1)
    {
      errorf("failure, dev=%s", dev->name);
      return -1;
    }
  }
  dev->flags &= ~NET_DEVICE_FLAG_UP;
  infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
  return 0;
}

int net_device_output(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst)
{
  if (!NET_DEVICE_IS_UP(dev))
  {
    errorf("device is not open, dev=%s", dev->name);
    return -1;
  }
  if (len > dev->mtu)
  {
    errorf("payload is too long, dev=%s, mtu=%u, len=%zu", dev->name, dev->mtu, len);
    return -1;
  }
  debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len)
      debugdump(data, len);
  if (dev->ops->transmit(dev, type, data, len, dst) == -1)
  {
    errorf("device transmit failure, dev=%s, len=%zu", dev->name, len);
    return -1;
  }
  return 0;
}

int net_input_handler(uint16_t type, const uint8_t *data, size_t len, struct net_device *dev)
{
  debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
  debugdump(data, len);
  return 0;
}

int net_run(void)
{
  struct net_device *dev;

  debugf("opening all devices...");
  for (dev = devices; dev; dev = dev->next)
  {
    net_device_open(dev);
  }
  debugf("running...");
  return 0;
}

void net_shutdown(void)
{
  struct net_device *dev;
  debugf("closing all devices...");
  for (dev = devices; dev; dev = dev->next)
  {
    net_device_close(dev);
  }
  debugf("...shutdown");
}

int net_init(void)
{
  /* do nothing */
  return 0;
}
