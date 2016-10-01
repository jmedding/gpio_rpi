defmodule ElixirAleRpiTest do
  use ExUnit.Case

  test "dth11h" do
    {:ok, pid} = GpioRpi.start_link 4, :input
    result = GpioRpi.dht11 pid
    assert result == :ok
  end
end
